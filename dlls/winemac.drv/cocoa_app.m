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
#import "cocoa_cursorclipping.h"
#import "cocoa_event.h"
#import "cocoa_window.h"

#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"


static NSString* const WineAppWaitQueryResponseMode = @"WineAppWaitQueryResponseMode";

// Private notifications that are reliably dispatched when a window is moved by dragging its titlebar.
// The object of the notification is the window being dragged.
// Available in macOS 10.12+
static NSString* const NSWindowWillStartDraggingNotification = @"NSWindowWillStartDraggingNotification";
static NSString* const NSWindowDidEndDraggingNotification = @"NSWindowDidEndDraggingNotification";

// Internal distributed notification to handle cooperative app activation in Sonoma.
static NSString* const WineAppWillActivateNotification = @"WineAppWillActivateNotification";
static NSString* const WineActivatingAppPIDKey = @"ActivatingAppPID";
static NSString* const WineActivatingAppPrefixKey = @"ActivatingAppPrefix";
static NSString* const WineActivatingAppConfigDirKey = @"ActivatingAppConfigDir";


int macdrv_err_on;


#if !defined(MAC_OS_X_VERSION_10_12) || MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_12
@interface NSWindow (WineAutoTabbingExtensions)

    + (void) setAllowsAutomaticWindowTabbing:(BOOL)allows;

@end
#endif


#if !defined(MAC_OS_VERSION_14_0) || MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_VERSION_14_0
@interface NSApplication (CooperativeActivationSelectorsForOldSDKs)

    - (void)activate;
    - (void)yieldActivationToApplication:(NSRunningApplication *)application;
    - (void)yieldActivationToApplicationWithBundleIdentifier:(NSString *)bundleIdentifier;

@end

@interface NSRunningApplication (CooperativeActivationSelectorsForOldSDKs)

    - (BOOL)activateFromApplication:(NSRunningApplication *)application
                            options:(NSApplicationActivationOptions)options;

@end
#endif


/***********************************************************************
 *              WineLocalizedString
 *
 * Look up a localized string by its ID in the dictionary.
 */
static NSString* WineLocalizedString(unsigned int stringID)
{
    return ((NSDictionary*)localized_strings)[@(stringID)];
}


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
    @synthesize lastSetCursorPositionTime;

    + (void) initialize
    {
        if (self == [WineApplicationController class])
        {
            NSDictionary<NSString *, id> *defaults =
            @{
                @"NSQuotedKeystrokeBinding" : @"",
                    @"NSRepeatCountBinding" : @"",
                @"ApplePressAndHoldEnabled" : @NO
            };

            [[NSUserDefaults standardUserDefaults] registerDefaults:defaults];

            if ([NSWindow respondsToSelector:@selector(setAllowsAutomaticWindowTabbing:)])
                [NSWindow setAllowsAutomaticWindowTabbing:NO];
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

            windowsBeingDragged = [[NSMutableSet alloc] init];

            // On macOS 10.12+, use notifications to more reliably detect when windows are being dragged.
            if ([NSProcessInfo instancesRespondToSelector:@selector(isOperatingSystemAtLeastVersion:)])
            {
                NSOperatingSystemVersion requiredVersion = { 10, 12, 0 };
                useDragNotifications = [[NSProcessInfo processInfo] isOperatingSystemAtLeastVersion:requiredVersion];
            }
            else
                useDragNotifications = NO;

            if (!requests || !requestsManipQueue || !eventQueues || !eventQueuesLock ||
                !keyWindows || !originalDisplayModes || !latentDisplayModes)
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
        [clipCursorHandler release];
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

    - (void) transformProcessToForeground:(BOOL)activateIfTransformed
    {
        if ([NSApp activationPolicy] != NSApplicationActivationPolicyRegular)
        {
            NSMenu* mainMenu;
            NSMenu* submenu;
            NSString* bundleName;
            NSString* title;
            NSMenuItem* item;

            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

            if (activateIfTransformed)
                [self tryToActivateIgnoringOtherApps:YES];

            if (!enable_app_nap)
            {
                [[[NSProcessInfo processInfo] beginActivityWithOptions:NSActivityUserInitiatedAllowingIdleSystemSleep
                                                                reason:@"Running Windows program"] retain]; // intentional leak
            }

            mainMenu = [[[NSMenu alloc] init] autorelease];

            // Application menu
            submenu = [[[NSMenu alloc] initWithTitle:WineLocalizedString(STRING_MENU_WINE)] autorelease];
            bundleName = [[NSBundle mainBundle] objectForInfoDictionaryKey:(NSString*)kCFBundleNameKey];

            if ([bundleName length])
                title = [NSString stringWithFormat:WineLocalizedString(STRING_MENU_ITEM_HIDE_APPNAME), bundleName];
            else
                title = WineLocalizedString(STRING_MENU_ITEM_HIDE);
            item = [submenu addItemWithTitle:title action:@selector(hide:) keyEquivalent:@""];

            item = [submenu addItemWithTitle:WineLocalizedString(STRING_MENU_ITEM_HIDE_OTHERS)
                                      action:@selector(hideOtherApplications:)
                               keyEquivalent:@"h"];
            [item setKeyEquivalentModifierMask:NSEventModifierFlagCommand | NSEventModifierFlagOption];

            item = [submenu addItemWithTitle:WineLocalizedString(STRING_MENU_ITEM_SHOW_ALL)
                                      action:@selector(unhideAllApplications:)
                               keyEquivalent:@""];

            [submenu addItem:[NSMenuItem separatorItem]];

            if ([bundleName length])
                title = [NSString stringWithFormat:WineLocalizedString(STRING_MENU_ITEM_QUIT_APPNAME), bundleName];
            else
                title = WineLocalizedString(STRING_MENU_ITEM_QUIT);
            item = [submenu addItemWithTitle:title action:@selector(terminate:) keyEquivalent:@"q"];
            [item setKeyEquivalentModifierMask:NSEventModifierFlagCommand | NSEventModifierFlagOption];
            item = [[[NSMenuItem alloc] init] autorelease];
            [item setTitle:WineLocalizedString(STRING_MENU_WINE)];
            [item setSubmenu:submenu];
            [mainMenu addItem:item];

            // Window menu
            submenu = [[[NSMenu alloc] initWithTitle:WineLocalizedString(STRING_MENU_WINDOW)] autorelease];
            [submenu addItemWithTitle:WineLocalizedString(STRING_MENU_ITEM_MINIMIZE)
                               action:@selector(performMiniaturize:)
                        keyEquivalent:@""];
            [submenu addItemWithTitle:WineLocalizedString(STRING_MENU_ITEM_ZOOM)
                               action:@selector(performZoom:)
                        keyEquivalent:@""];
            item = [submenu addItemWithTitle:WineLocalizedString(STRING_MENU_ITEM_ENTER_FULL_SCREEN)
                                      action:@selector(toggleFullScreen:)
                               keyEquivalent:@"f"];
            [item setKeyEquivalentModifierMask:NSEventModifierFlagCommand |
                                               NSEventModifierFlagOption |
                                               NSEventModifierFlagControl];
            [submenu addItem:[NSMenuItem separatorItem]];
            [submenu addItemWithTitle:WineLocalizedString(STRING_MENU_ITEM_BRING_ALL_TO_FRONT)
                               action:@selector(arrangeInFront:)
                        keyEquivalent:@""];
            item = [[[NSMenuItem alloc] init] autorelease];
            [item setTitle:WineLocalizedString(STRING_MENU_WINDOW)];
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
                @autoreleasepool
                {
                    NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                                        untilDate:timeout
                                                           inMode:NSDefaultRunLoopMode
                                                          dequeue:YES];
                    if (event)
                        [NSApp sendEvent:event];
                }
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

    static BOOL EqualInputSource(TISInputSourceRef source1, TISInputSourceRef source2)
    {
        if (!source1 && !source2)
            return TRUE;
        if (!source1 || !source2)
            return FALSE;
        return CFEqual(source1, source2);
    }

    - (void) keyboardSelectionDidChange:(BOOL)force
    {
        TISInputSourceRef inputSource, inputSourceLayout;

        if (!force)
        {
            NSTextInputContext* context = [NSTextInputContext currentInputContext];
            if (!context || ![context client])
                return;
        }

        inputSource = TISCopyCurrentKeyboardInputSource();
        inputSourceLayout = TISCopyCurrentKeyboardLayoutInputSource();
        if (!force && EqualInputSource(inputSource, lastKeyboardInputSource) &&
            EqualInputSource(inputSourceLayout, lastKeyboardLayoutInputSource))
        {
            if (inputSource) CFRelease(inputSource);
            if (inputSourceLayout) CFRelease(inputSourceLayout);
            return;
        }

        if (lastKeyboardInputSource)
            CFRelease(lastKeyboardInputSource);
        lastKeyboardInputSource = inputSource;
        if (lastKeyboardLayoutInputSource)
            CFRelease(lastKeyboardLayoutInputSource);
        lastKeyboardLayoutInputSource = inputSourceLayout;

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
                event->keyboard_changed.input_source = (TISInputSourceRef)CFRetain(inputSource);

                if (event->keyboard_changed.uchr)
                {
                    [eventQueuesLock lock];

                    for (queue in eventQueues)
                        [queue postEvent:event];

                    [eventQueuesLock unlock];
                }

                macdrv_release_event(event);
            }
        }
    }

    - (void) keyboardSelectionDidChange
    {
        [self keyboardSelectionDidChange:NO];
    }

    - (void) setKeyboardType:(CGEventSourceKeyboardType)newType
    {
        if (newType != keyboardType)
        {
            keyboardType = newType;
            [self keyboardSelectionDidChange:YES];
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

                primaryScreenHeight = NSHeight([screens[0] frame]);
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
        rect->origin.y = NSMaxY([[NSScreen screens][0] frame]) - NSMaxY(*rect);
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
        __block NSInteger minFloatingLevel = NSFloatingWindowLevel;
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

            if (window.floating)
            {
                if (minFloatingLevel <= maxNonfloatingLevel)
                    minFloatingLevel = maxNonfloatingLevel + 1;
                if (newLevel < minFloatingLevel)
                    newLevel = minFloatingLevel;
            }

            if (newLevel < maxLevel)
                newLevel = maxLevel;
            else
                maxLevel = newLevel;

            if (!window.floating && maxNonfloatingLevel < newLevel)
                maxNonfloatingLevel = newLevel;

            if (newLevel != origLevel)
            {
                [window setLevel:newLevel];

                if (origLevel < newLevel)
                {
                    // If we increased the level, the window should be toward the
                    // back of its new level (but still ahead of the previous
                    // windows we did this to).
                    if (prev)
                        [window orderWindow:NSWindowAbove relativeTo:[prev windowNumber]];
                    else
                        [window orderBack:nil];
                }
                else
                {
                    // If we decreased the level, we want the window at the top
                    // of its new level. -setLevel: is documented to do that on
                    // its own, but that's buggy on Ventura. Since we're looping
                    // back-to-front here, -orderFront: will do the right thing.
                    [window orderFront:nil];
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
        if (CGDisplayModeGetPixelWidth(mode1) != CGDisplayModeGetPixelWidth(mode2)) return FALSE;
        if (CGDisplayModeGetPixelHeight(mode1) != CGDisplayModeGetPixelHeight(mode2)) return FALSE;

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

    - (NSArray*)modesMatchingMode:(CGDisplayModeRef)mode forDisplay:(CGDirectDisplayID)displayID
    {
        NSMutableArray* ret = [NSMutableArray array];
        NSDictionary* options = @{ (NSString*)kCGDisplayShowDuplicateLowResolutionModes: @YES };

        NSArray *modes = [(NSArray*)CGDisplayCopyAllDisplayModes(displayID, (CFDictionaryRef)options) autorelease];
        for (id candidateModeObject in modes)
        {
            CGDisplayModeRef candidateMode = (CGDisplayModeRef)candidateModeObject;
            if ([self mode:candidateMode matchesMode:mode])
                [ret addObject:candidateModeObject];
        }
        return ret;
    }

    - (BOOL) setMode:(CGDisplayModeRef)mode forDisplay:(CGDirectDisplayID)displayID
    {
        BOOL ret = FALSE;
        NSNumber* displayIDKey = [NSNumber numberWithUnsignedInt:displayID];
        CGDisplayModeRef originalMode;

        originalMode = (CGDisplayModeRef)originalDisplayModes[displayIDKey];

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
                for (id modeObject in [self modesMatchingMode:mode forDisplay:displayID])
                {
                    mode = (CGDisplayModeRef)modeObject;
                    if (CGDisplaySetDisplayMode(displayID, mode, NULL) == CGDisplayNoErr)
                    {
                        [originalDisplayModes removeObjectForKey:displayIDKey];
                        ret = TRUE;
                        break;
                    }
                }
            }
        }
        else
        {
            CGDisplayModeRef currentMode;
            NSArray* modes;

            currentMode = CGDisplayModeRetain((CGDisplayModeRef)latentDisplayModes[displayIDKey]);
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

            modes = [self modesMatchingMode:mode forDisplay:displayID];
            if (!modes.count)
                return FALSE;

            [self transformProcessToForeground:YES];

            BOOL active = [NSApp isActive];

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
                    {
                        for (id modeObject in modes)
                        {
                            mode = (CGDisplayModeRef)modeObject;
                            if (CGDisplaySetDisplayMode(displayID, mode, NULL) == CGDisplayNoErr)
                            {
                                ret = TRUE;
                                break;
                            }
                        }
                    }
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
        NSDictionary* frame = cursorFrames[cursorFrame];
        CGImageRef cgimage = (CGImageRef)frame[@"image"];
        CGSize size = CGSizeMake(CGImageGetWidth(cgimage), CGImageGetHeight(cgimage));
        NSImage* image = [[NSImage alloc] initWithCGImage:cgimage size:NSSizeFromCGSize(cgsize_mac_from_win(size))];
        CFDictionaryRef hotSpotDict = (CFDictionaryRef)frame[@"hotSpot"];
        CGPoint hotSpot;

        if (!CGPointMakeWithDictionaryRepresentation(hotSpotDict, &hotSpot))
            hotSpot = CGPointZero;
        hotSpot = cgpoint_mac_from_win(hotSpot);
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

        frame = cursorFrames[cursorFrame];
        duration = [frame[@"duration"] doubleValue];
        date = [[theTimer fireDate] dateByAddingTimeInterval:duration];
        [cursorTimer setFireDate:date];
    }

    - (void) setCursorWithFrames:(NSArray*)frames
    {
        if (self.cursorFrames == frames || [self.cursorFrames isEqualToArray:frames])
            return;

        self.cursorFrames = frames;
        cursorFrame = 0;
        [cursorTimer invalidate];
        self.cursorTimer = nil;

        if ([frames count])
        {
            if ([frames count] > 1)
            {
                NSDictionary* frame = frames[0];
                NSTimeInterval duration = [frame[@"duration"] doubleValue];
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

    - (BOOL) setCursorPosition:(CGPoint)pos
    {
        BOOL ret;

        if ([windowsBeingDragged count])
            ret = FALSE;
        else if (self.clippingCursor && [clipCursorHandler respondsToSelector:@selector(setCursorPosition:)])
            ret = [clipCursorHandler setCursorPosition:pos];
        else
        {
            if (self.clippingCursor)
                [clipCursorHandler clipCursorLocation:&pos];

            // Annoyingly, CGWarpMouseCursorPosition() effectively disassociates
            // the mouse from the cursor position for 0.25 seconds.  This means
            // that mouse movement during that interval doesn't move the cursor
            // and events carry a constant location (the warped-to position)
            // even though they have delta values.  For apps which warp the
            // cursor frequently (like after every mouse move), this makes
            // cursor movement horribly laggy and jerky, as only a fraction of
            // mouse move events have any effect.
            //
            // On some versions of OS X, it's sufficient to forcibly reassociate
            // the mouse and cursor position.  On others, it's necessary to set
            // the local events suppression interval to 0 for the warp.  That's
            // deprecated, but I'm not aware of any other way.  For good
            // measure, we do both.
            CGSetLocalEventsSuppressionInterval(0);
            ret = (CGWarpMouseCursorPosition(pos) == kCGErrorSuccess);
            CGSetLocalEventsSuppressionInterval(0.25);
            if (ret)
            {
                lastSetCursorPositionTime = [[NSProcessInfo processInfo] systemUptime];

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
                [queue discardEventsMatchingMask:event_mask_for_type(MOUSE_MOVED_RELATIVE) |
                                                 event_mask_for_type(MOUSE_MOVED_ABSOLUTE)
                                       forWindow:nil];
                [queue resetMouseEventPositions:pos];
            }
            [eventQueuesLock unlock];
        }

        return ret;
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
        if (!clipCursorHandler) {
            if (use_confinement_cursor_clipping && [WineConfinementClipCursorHandler isAvailable])
                clipCursorHandler = [[WineConfinementClipCursorHandler alloc] init];
            else
                clipCursorHandler = [[WineEventTapClipCursorHandler alloc] init];
        }

        if (self.clippingCursor && CGRectEqualToRect(rect, clipCursorHandler.cursorClipRect))
            return TRUE;

        if (![clipCursorHandler startClippingCursor:rect])
            return FALSE;

        [self setCursorPosition:NSPointToCGPoint([self flippedMouseLocation:[NSEvent mouseLocation]])];

        [self updateWindowsForCursorClipping];

        return TRUE;
    }

    - (BOOL) stopClippingCursor
    {
        if (!self.clippingCursor)
            return TRUE;

        if (![clipCursorHandler stopClippingCursor])
            return FALSE;

        lastSetCursorPositionTime = [[NSProcessInfo processInfo] systemUptime];

        [self updateWindowsForCursorClipping];

        return TRUE;
    }

    - (BOOL) clippingCursor
    {
        return clipCursorHandler.clippingCursor;
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

    - (void) window:(WineWindow*)window isBeingDragged:(BOOL)dragged
    {
        if (dragged)
            [windowsBeingDragged addObject:window];
        else
            [windowsBeingDragged removeObject:window];
    }

    - (void) windowWillOrderOut:(WineWindow*)window
    {
        if ([windowsBeingDragged containsObject:window])
        {
            [self window:window isBeingDragged:NO];

            macdrv_event* event = macdrv_create_event(WINDOW_DRAG_END, window);
            [window.queue postEvent:event];
            macdrv_release_event(event);
        }
    }

    - (BOOL) isAnyWineWindowVisible
    {
        for (WineWindow* w in [NSApp windows])
        {
            if ([w isKindOfClass:[WineWindow class]] && ![w isMiniaturized] && [w isVisible] && [w presentsVisibleContent])
                return YES;
        }

        return NO;
    }

    - (void) handleWindowDrag:(WineWindow*)window begin:(BOOL)begin
    {
        macdrv_event* event;
        int eventType;

        if (begin)
        {
            [windowsBeingDragged addObject:window];
            eventType = WINDOW_DRAG_BEGIN;
        }
        else
        {
            [windowsBeingDragged removeObject:window];
            eventType = WINDOW_DRAG_END;
        }

        event = macdrv_create_event(eventType, window);
        if (eventType == WINDOW_DRAG_BEGIN)
            event->window_drag_begin.no_activate = [NSEvent wine_commandKeyDown];
        [window.queue postEvent:event];
        macdrv_release_event(event);
    }

    - (void) handleMouseMove:(NSEvent*)anEvent
    {
        WineWindow* targetWindow;
        BOOL drag = [anEvent type] != NSEventTypeMouseMoved;

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
                if (!self.clippingCursor || CGRectContainsPoint(clipCursorHandler.cursorClipRect, computedPoint))
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
                if (self.clippingCursor)
                    [clipCursorHandler clipCursorLocation:&point];
                point = cgpoint_win_from_mac(point);

                event = macdrv_create_event(MOUSE_MOVED_ABSOLUTE, targetWindow);
                event->mouse_moved.x = floor(point.x);
                event->mouse_moved.y = floor(point.y);

                mouseMoveDeltaX = 0;
                mouseMoveDeltaY = 0;
            }
            else
            {
                double scale = retina_on ? 2 : 1;

                /* Add event delta to accumulated delta error */
                /* deltaY is already flipped */
                mouseMoveDeltaX += [anEvent deltaX];
                mouseMoveDeltaY += [anEvent deltaY];

                event = macdrv_create_event(MOUSE_MOVED_RELATIVE, targetWindow);
                event->mouse_moved.x = mouseMoveDeltaX * scale;
                event->mouse_moved.y = mouseMoveDeltaY * scale;

                /* Keep the remainder after integer truncation. */
                mouseMoveDeltaX -= event->mouse_moved.x / scale;
                mouseMoveDeltaY -= event->mouse_moved.y / scale;
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
        WineWindow* windowBroughtForward = nil;
        BOOL process = FALSE;

        if ([window isKindOfClass:[WineWindow class]] &&
            type == NSEventTypeLeftMouseDown &&
            ![theEvent wine_commandKeyDown])
        {
            NSWindowButton windowButton;

            windowBroughtForward = window;

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
                        windowBroughtForward = nil;
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
            BOOL pressed = (type == NSEventTypeLeftMouseDown ||
                            type == NSEventTypeRightMouseDown ||
                            type == NSEventTypeOtherMouseDown);
            CGPoint pt = CGEventGetLocation([theEvent CGEvent]);

            if (self.clippingCursor)
                [clipCursorHandler clipCursorLocation:&pt];

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
                    if (process && [window styleMask] & NSWindowStyleMaskResizable)
                    {
                        // Ignore clicks in the grow box (resize widget).
                        HIPoint origin = { 0, 0 };
                        HIThemeGrowBoxDrawInfo info = { 0 };
                        HIRect bounds;
                        OSStatus status;

                        info.kind = kHIThemeGrowBoxKindNormal;
                        info.direction = kThemeGrowRight | kThemeGrowDown;
                        if ([window styleMask] & NSWindowStyleMaskUtilityWindow)
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

                pt = cgpoint_win_from_mac(pt);

                event = macdrv_create_event(MOUSE_BUTTON, window);
                event->mouse_button.button = [theEvent buttonNumber];
                event->mouse_button.pressed = pressed;
                event->mouse_button.x = floor(pt.x);
                event->mouse_button.y = floor(pt.y);
                event->mouse_button.time_ms = [self ticksForEventTime:[theEvent timestamp]];

                [window.queue postEvent:event];

                macdrv_release_event(event);
            }
        }

        if (windowBroughtForward)
        {
            WineWindow* ancestor = [windowBroughtForward ancestorWineWindow];
            NSInteger ancestorNumber = [ancestor windowNumber];
            NSInteger ancestorLevel = [ancestor level];

            for (NSNumber* windowNumberObject in [NSWindow windowNumbersWithOptions:0])
            {
                NSInteger windowNumber = [windowNumberObject integerValue];
                if (windowNumber == ancestorNumber)
                    break;
                WineWindow* otherWindow = (WineWindow*)[NSApp windowWithWindowNumber:windowNumber];
                if ([otherWindow isKindOfClass:[WineWindow class]] && [otherWindow screen] &&
                    [otherWindow level] <= ancestorLevel && otherWindow == [otherWindow ancestorWineWindow])
                {
                    [ancestor postBroughtForwardEvent];
                    break;
                }
            }
            if (!process && ![windowBroughtForward isKeyWindow] && !windowBroughtForward.disabled && !windowBroughtForward.noForeground)
                [self windowGotFocus:windowBroughtForward];
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

            if (self.clippingCursor)
                [clipCursorHandler clipCursorLocation:&pt];

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
                double x, y;
                BOOL continuous = FALSE;

                pt = cgpoint_win_from_mac(pt);

                event = macdrv_create_event(MOUSE_SCROLL, window);
                event->mouse_scroll.x = floor(pt.x);
                event->mouse_scroll.y = floor(pt.y);
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
                x *= 6;
                y *= 6;

                if (use_precise_scrolling)
                {
                    event->mouse_scroll.x_scroll = x;
                    event->mouse_scroll.y_scroll = y;

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
                }
                else
                {
                    /* If it's been a while since the last scroll event or if the scrolling has
                       reversed direction, reset the accumulated scroll value. */
                    if ([theEvent timestamp] - lastScrollTime > 1)
                        accumScrollX = accumScrollY = 0;
                    else
                    {
                        /* The accumulated scroll value is in the opposite direction/sign of the last
                           scroll.  That's because it's the "debt" resulting from over-scrolling in
                           that direction.  We accumulate by adding in the scroll amount and then, if
                           it has the same sign as the scroll value, we subtract any whole or partial
                           WHEEL_DELTAs, leaving it 0 or the opposite sign.  So, the user switched
                           scroll direction if the accumulated debt and the new scroll value have the
                           same sign. */
                        if ((accumScrollX < 0 && x < 0) || (accumScrollX > 0 && x > 0))
                            accumScrollX = 0;
                        if ((accumScrollY < 0 && y < 0) || (accumScrollY > 0 && y > 0))
                            accumScrollY = 0;
                    }
                    lastScrollTime = [theEvent timestamp];

                    accumScrollX += x;
                    accumScrollY += y;

                    if (accumScrollX > 0 && x > 0)
                        event->mouse_scroll.x_scroll = 120 * ceil(accumScrollX / 120);
                    if (accumScrollX < 0 && x < 0)
                        event->mouse_scroll.x_scroll = 120 * -ceil(-accumScrollX / 120);
                    if (accumScrollY > 0 && y > 0)
                        event->mouse_scroll.y_scroll = 120 * ceil(accumScrollY / 120);
                    if (accumScrollY < 0 && y < 0)
                        event->mouse_scroll.y_scroll = 120 * -ceil(-accumScrollY / 120);

                    accumScrollX -= event->mouse_scroll.x_scroll;
                    accumScrollY -= event->mouse_scroll.y_scroll;
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

        if (type == NSEventTypeFlagsChanged)
            self.lastFlagsChanged = anEvent;
        else if (type == NSEventTypeMouseMoved || type == NSEventTypeLeftMouseDragged ||
                 type == NSEventTypeRightMouseDragged || type == NSEventTypeOtherMouseDragged)
        {
            [self handleMouseMove:anEvent];
            ret = mouseCaptureWindow && ![windowsBeingDragged count];
        }
        else if (type == NSEventTypeLeftMouseDown || type == NSEventTypeLeftMouseUp ||
                 type == NSEventTypeRightMouseDown || type == NSEventTypeRightMouseUp ||
                 type == NSEventTypeOtherMouseDown || type == NSEventTypeOtherMouseUp)
        {
            [self handleMouseButton:anEvent];
            ret = mouseCaptureWindow && ![windowsBeingDragged count];
        }
        else if (type == NSEventTypeScrollWheel)
        {
            [self handleScrollWheel:anEvent];
            ret = mouseCaptureWindow != nil;
        }
        else if (type == NSEventTypeKeyDown)
        {
            // -[NSApplication sendEvent:] seems to consume presses of the Help
            // key (Insert key on PC keyboards), so we have to bypass it and
            // send the event directly to the window.
            if (anEvent.keyCode == kVK_Help)
            {
                [anEvent.window sendEvent:anEvent];
                ret = TRUE;
            }
        }
        else if (type == NSEventTypeKeyUp)
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
        else if (!useDragNotifications && type == NSEventTypeAppKitDefined)
        {
            WineWindow *window = (WineWindow *)[anEvent window];
            short subtype = [anEvent subtype];

            // These subtypes are not documented but they appear to mean
            // "a window is being dragged" and "a window is no longer being
            // dragged", respectively.
            if ((subtype == 20 || subtype == 21) && [window isKindOfClass:[WineWindow class]])
                [self handleWindowDrag:window begin:(subtype == 20)];
        }

        return ret;
    }

    - (void) didSendEvent:(NSEvent*)anEvent
    {
        NSEventType type = [anEvent type];

        if (type == NSEventTypeKeyDown && ![anEvent isARepeat] && [anEvent keyCode] == kVK_Tab)
        {
            NSUInteger modifiers = [anEvent modifierFlags];
            if ((modifiers & NSEventModifierFlagCommand) &&
                !(modifiers & (NSEventModifierFlagControl | NSEventModifierFlagOption)))
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
        }];

        if (useDragNotifications) {
            [nc addObserverForName:NSWindowWillStartDraggingNotification
                            object:nil
                             queue:[NSOperationQueue mainQueue]
                        usingBlock:^(NSNotification *note){
                NSWindow* window = [note object];
                if ([window isKindOfClass:[WineWindow class]])
                    [self handleWindowDrag:(WineWindow *)window begin:YES];
            }];

            [nc addObserverForName:NSWindowDidEndDraggingNotification
                            object:nil
                             queue:[NSOperationQueue mainQueue]
                        usingBlock:^(NSNotification *note){
                NSWindow* window = [note object];
                if ([window isKindOfClass:[WineWindow class]])
                    [self handleWindowDrag:(WineWindow *)window begin:NO];
            }];
        }

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

        if ([NSApplication instancesRespondToSelector:@selector(yieldActivationToApplication:)])
        {
            /* App activation cooperation, starting in macOS 14 Sonoma. */
            [dnc addObserver:self
                    selector:@selector(otherWineAppWillActivate:)
                        name:WineAppWillActivateNotification
                      object:nil
          suspensionBehavior:NSNotificationSuspensionBehaviorDeliverImmediately];
        }
    }

    - (void) otherWineAppWillActivate:(NSNotification *)note
    {
        NSProcessInfo *ourProcess;
        pid_t otherPID;
        NSString *ourConfigDir, *otherConfigDir, *ourPrefix, *otherPrefix;
        NSRunningApplication *otherApp;

        /* No point in yielding if we're not the foreground app. */
        if (![NSApp isActive]) return;

        /* Ignore requests from ourself, dead processes, and other prefixes. */
        ourProcess = [NSProcessInfo processInfo];
        otherPID = [note.userInfo[WineActivatingAppPIDKey] integerValue];
        if (otherPID == ourProcess.processIdentifier) return;

        otherApp = [NSRunningApplication runningApplicationWithProcessIdentifier:otherPID];
        if (!otherApp) return;

        ourConfigDir = ourProcess.environment[@"WINECONFIGDIR"];
        otherConfigDir = note.userInfo[WineActivatingAppConfigDirKey];
        if (ourConfigDir.length && otherConfigDir.length &&
            ![ourConfigDir isEqualToString:otherConfigDir])
        {
            return;
        }

        ourPrefix = ourProcess.environment[@"WINEPREFIX"];
        otherPrefix = note.userInfo[WineActivatingAppPrefixKey];
        if (ourPrefix.length && otherPrefix.length &&
            ![ourPrefix isEqualToString:otherPrefix])
        {
            return;
        }

        /* There's a race condition here. The requesting app sends out
           WineAppWillActivateNotification and then activates itself, but since
           distributed notifications are asynchronous, we may not have yielded
           in time. So we call activateFromApplication: on the other app here,
           which will work around that race if it happened. If we didn't hit the
           race, the activateFromApplication: call will be a no-op. */

        /* We only add this observer if NSApplication responds to the yield
           methods, so they're safe to call without checking here. */
        [NSApp yieldActivationToApplication:otherApp];
        [otherApp activateFromApplication:[NSRunningApplication currentApplication]
                                  options:0];
    }

    - (void) tryToActivateIgnoringOtherApps:(BOOL)ignore
    {
        NSProcessInfo *processInfo;
        NSString *configDir, *prefix;
        NSDictionary *userInfo;

        if ([NSApp isActive]) return;  /* Nothing to do. */

        if (!ignore ||
            ![NSApplication instancesRespondToSelector:@selector(yieldActivationToApplication:)])
        {
            /* Either we don't need to force activation, or the OS is old enough
               that this is our only option. */
            [NSApp activateIgnoringOtherApps:ignore];
            return;
        }

        /* Ask other Wine apps to yield activation to us. */
        processInfo = [NSProcessInfo processInfo];
        configDir = processInfo.environment[@"WINECONFIGDIR"];
        prefix = processInfo.environment[@"WINEPREFIX"];
        userInfo = @{
            WineActivatingAppPIDKey: @(processInfo.processIdentifier),
            WineActivatingAppPrefixKey: prefix ? prefix : @"",
            WineActivatingAppConfigDirKey: configDir ? configDir : @""
        };

        [[NSDistributedNotificationCenter defaultCenter]
            postNotificationName:WineAppWillActivateNotification
                          object:nil
                        userInfo:userInfo
              deliverImmediately:YES];

        /* This is racy. See the note in otherWineAppWillActivate:. */
        [NSApp activate];
     }

    static BOOL InputSourceShouldBeIgnored(TISInputSourceRef inputSource)
    {
        /* Certain system utilities are technically input sources, but we
           shouldn't consider them as such for our purposes. */
        static CFStringRef ignoredIDs[] = {
            /* The "Emoji & Symbols" palette. */
            CFSTR("com.apple.CharacterPaletteIM"),
            /* The on-screen keyboard and accessibility panel. */
            CFSTR("com.apple.inputmethod.AssistiveControl"),
            /* The popup for accented characters when you hold down a key. */
            CFSTR("com.apple.PressAndHold"),
            /* Emoji list on MacBooks with the Touch Bar. */
            CFSTR("com.apple.inputmethod.EmojiFunctionRowItem"),
            /* Dictation. Ideally this would actually receive key events, since
               escape cancels it, but it remains a "selected" input source even
               when not active, so we need to ignore it to avoid incorrectly
               sending input to it. */
            CFSTR("com.apple.inputmethod.ironwood"),
        };

        CFStringRef sourceID = TISGetInputSourceProperty(inputSource, kTISPropertyInputSourceID);
        for (int i = 0; i < sizeof(ignoredIDs) / sizeof(CFStringRef); i++)
        {
            if (CFEqual(sourceID, ignoredIDs[i]))
                return YES;
        }

        return NO;
    }

    - (BOOL) inputSourceIsInputMethod
    {
        static dispatch_once_t onceToken;
        static CFDictionaryRef filterDict;
        CFArrayRef enabledSources;
        CFIndex i;
        BOOL ret = NO;

        /* There may be multiple active ("selected") input sources, but there is
           always exactly one selected keyboard input source. For instance,
           handwriting methods are active simultaneously with a keyboard source.
           As the name implies, TISCopyCurrentKeyboardInputSource only returns
           the keyboard source, so it's not sufficient for our needs. We use
           TISCreateInputSourceList instead to find all selected sources. */
        dispatch_once(&onceToken, ^{
            filterDict = CFDictionaryCreate(NULL, (const void **)&kTISPropertyInputSourceIsSelected, (const void **)&kCFBooleanTrue, 1,
                                            &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

        });
        enabledSources = TISCreateInputSourceList(filterDict, false);
        for (i = 0; i < CFArrayGetCount(enabledSources); i++)
        {
            TISInputSourceRef source = (TISInputSourceRef)CFArrayGetValueAtIndex(enabledSources, i);
            CFStringRef type = TISGetInputSourceProperty(source, kTISPropertyInputSourceType);

            /* kTISTypeKeyboardLayout is for physical keyboards. Any type other
               than that is an IME. */
            if (!CFEqual(type, kTISTypeKeyboardLayout) && !InputSourceShouldBeIgnored(source))
            {
                ret = YES;
                break;
            }
        }

        CFRelease(enabledSources);
        return ret;
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
        WineWindow *bestOption = nil;

        if ([self isAnyWineWindowVisible])
            return;

        for (WineWindow *window in [NSApp windows])
        {
            if (![window isKindOfClass:[WineWindow class]] || ![window isMiniaturized])
                continue;

            bestOption = window;

            /* Prefer any window that would actually show something. */
            if ([window presentsVisibleContent])
                break;
        }

        [bestOption deminiaturize:self];
    }

    - (void) setRetinaMode:(int)mode
    {
        retina_on = mode;

        [clipCursorHandler setRetinaMode:mode];

        for (WineWindow* window in [NSApp windows])
        {
            if ([window isKindOfClass:[WineWindow class]])
                [window setRetinaMode:mode];
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
            CGDisplayModeRef mode = (CGDisplayModeRef)modesToRealize[displayID];
            [self setMode:mode forDisplay:[displayID unsignedIntValue]];
        }

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

    - (void)applicationWillBecomeActive:(NSNotification *)notification
    {
        macdrv_event* event = macdrv_create_event(APP_ACTIVATED, nil);
        event->deliver = 1;

        [eventQueuesLock lock];
        for (WineEventQueue* queue in eventQueues)
            [queue postEvent:event];
        [eventQueuesLock unlock];

        macdrv_release_event(event);
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
@autoreleasepool
{
    WineApplicationController* controller = [WineApplicationController sharedController];

    for (;;)
    {
        @autoreleasepool
        {
            __block dispatch_block_t block;

            dispatch_sync(controller->requestsManipQueue, ^{
                if ([controller->requests count])
                {
                    block = (dispatch_block_t)[controller->requests[0] retain];
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
@autoreleasepool
{
    NSString* message = [[NSString alloc] initWithFormat:format arguments:args];
    fprintf(stderr, "err:%s:%s", func, [message UTF8String]);
    [message release];
}
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
            if (input_source)
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
        *pos = cgpoint_win_from_mac(NSPointToCGPoint(location));
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
        ret = [[WineApplicationController sharedController] setCursorPosition:cgpoint_mac_from_win(pos)];
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
int macdrv_clip_cursor(CGRect r)
{
    __block int ret;

    OnMainThread(^{
        WineApplicationController* controller = [WineApplicationController sharedController];
        BOOL clipping = FALSE;
        CGRect rect = r;

        if (!CGRectIsInfinite(rect))
            rect = cgrect_mac_from_win(rect);

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

void macdrv_set_cocoa_retina_mode(int new_mode)
{
    OnMainThread(^{
        [[WineApplicationController sharedController] setRetinaMode:new_mode];
    });
}

int macdrv_is_any_wine_window_visible(void)
{
    __block int ret = FALSE;

    OnMainThread(^{
        ret = [[WineApplicationController sharedController] isAnyWineWindowVisible];
    });

    return ret;
}
