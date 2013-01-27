/*
 * MACDRV Cocoa window code
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

#import "cocoa_window.h"

#include "macdrv_cocoa.h"
#import "cocoa_app.h"
#import "cocoa_event.h"


static NSUInteger style_mask_for_features(const struct macdrv_window_features* wf)
{
    NSUInteger style_mask;

    if (wf->title_bar)
    {
        style_mask = NSTitledWindowMask;
        if (wf->close_button) style_mask |= NSClosableWindowMask;
        if (wf->minimize_button) style_mask |= NSMiniaturizableWindowMask;
        if (wf->resizable) style_mask |= NSResizableWindowMask;
        if (wf->utility) style_mask |= NSUtilityWindowMask;
    }
    else style_mask = NSBorderlessWindowMask;

    return style_mask;
}


static BOOL frame_intersects_screens(NSRect frame, NSArray* screens)
{
    NSScreen* screen;
    for (screen in screens)
    {
        if (NSIntersectsRect(frame, [screen frame]))
            return TRUE;
    }
    return FALSE;
}


@interface WineContentView : NSView
@end


@interface WineWindow ()

@property (nonatomic) BOOL disabled;
@property (nonatomic) BOOL noActivate;
@property (nonatomic) BOOL floating;
@property (retain, nonatomic) NSWindow* latentParentWindow;

@property (nonatomic) void* hwnd;
@property (retain, readwrite, nonatomic) WineEventQueue* queue;

@property (nonatomic) void* surface;
@property (nonatomic) pthread_mutex_t* surface_mutex;

@property (copy, nonatomic) NSBezierPath* shape;
@property (nonatomic) BOOL shapeChangedSinceLastDraw;
@property (readonly, nonatomic) BOOL needsTransparency;

@property (nonatomic) BOOL colorKeyed;
@property (nonatomic) CGFloat colorKeyRed, colorKeyGreen, colorKeyBlue;
@property (nonatomic) BOOL usePerPixelAlpha;

    + (void) flipRect:(NSRect*)rect;

@end


@implementation WineContentView

    - (BOOL) isFlipped
    {
        return YES;
    }

    - (void) drawRect:(NSRect)rect
    {
        WineWindow* window = (WineWindow*)[self window];

        if (window.surface && window.surface_mutex &&
            !pthread_mutex_lock(window.surface_mutex))
        {
            const CGRect* rects;
            int count;

            if (!get_surface_region_rects(window.surface, &rects, &count) || count)
            {
                CGRect imageRect;
                CGImageRef image;

                imageRect = NSRectToCGRect(rect);
                image = create_surface_image(window.surface, &imageRect, FALSE);

                if (image)
                {
                    CGContextRef context;

                    if (rects && count)
                    {
                        NSBezierPath* surfaceClip = [NSBezierPath bezierPath];
                        int i;
                        for (i = 0; i < count; i++)
                            [surfaceClip appendBezierPathWithRect:NSRectFromCGRect(rects[i])];
                        [surfaceClip addClip];
                    }

                    [window.shape addClip];

                    if (window.colorKeyed)
                    {
                        CGImageRef maskedImage;
                        CGFloat components[] = { window.colorKeyRed - 0.5, window.colorKeyRed + 0.5,
                                                 window.colorKeyGreen - 0.5, window.colorKeyGreen + 0.5,
                                                 window.colorKeyBlue - 0.5, window.colorKeyBlue + 0.5 };
                        maskedImage = CGImageCreateWithMaskingColors(image, components);
                        if (maskedImage)
                        {
                            CGImageRelease(image);
                            image = maskedImage;
                        }
                    }

                    context = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
                    CGContextSetBlendMode(context, kCGBlendModeCopy);
                    CGContextDrawImage(context, imageRect, image);

                    CGImageRelease(image);

                    if (window.shapeChangedSinceLastDraw || window.colorKeyed ||
                        window.usePerPixelAlpha)
                    {
                        window.shapeChangedSinceLastDraw = FALSE;
                        [window invalidateShadow];
                    }
                }
            }

            pthread_mutex_unlock(window.surface_mutex);
        }
    }

    /* By default, NSView will swallow right-clicks in an attempt to support contextual
       menus.  We need to bypass that and allow the event to make it to the window. */
    - (void) rightMouseDown:(NSEvent*)theEvent
    {
        [[self window] rightMouseDown:theEvent];
    }

@end


@implementation WineWindow

    @synthesize disabled, noActivate, floating, latentParentWindow, hwnd, queue;
    @synthesize surface, surface_mutex;
    @synthesize shape, shapeChangedSinceLastDraw;
    @synthesize colorKeyed, colorKeyRed, colorKeyGreen, colorKeyBlue;
    @synthesize usePerPixelAlpha;

    + (WineWindow*) createWindowWithFeatures:(const struct macdrv_window_features*)wf
                                 windowFrame:(NSRect)window_frame
                                        hwnd:(void*)hwnd
                                       queue:(WineEventQueue*)queue
    {
        WineWindow* window;
        WineContentView* contentView;

        [self flipRect:&window_frame];

        window = [[[self alloc] initWithContentRect:window_frame
                                          styleMask:style_mask_for_features(wf)
                                            backing:NSBackingStoreBuffered
                                              defer:YES] autorelease];

        if (!window) return nil;
        window->normalStyleMask = [window styleMask];

        /* Standardize windows to eliminate differences between titled and
           borderless windows and between NSWindow and NSPanel. */
        [window setHidesOnDeactivate:NO];
        [window setReleasedWhenClosed:NO];

        [window disableCursorRects];
        [window setShowsResizeIndicator:NO];
        [window setHasShadow:wf->shadow];
        [window setColorSpace:[NSColorSpace genericRGBColorSpace]];
        [window setDelegate:window];
        window.hwnd = hwnd;
        window.queue = queue;

        contentView = [[[WineContentView alloc] initWithFrame:NSZeroRect] autorelease];
        if (!contentView)
            return nil;
        [contentView setAutoresizesSubviews:NO];

        [window setContentView:contentView];

        /* In case Cocoa adjusted the frame we tried to set, generate a frame-changed
           event.  The back end will ignore it if nothing actually changed. */
        [window windowDidResize:nil];

        return window;
    }

    - (void) dealloc
    {
        [queue release];
        [latentParentWindow release];
        [shape release];
        [super dealloc];
    }

    + (void) flipRect:(NSRect*)rect
    {
        rect->origin.y = NSMaxY([[[NSScreen screens] objectAtIndex:0] frame]) - NSMaxY(*rect);
    }

    - (void) adjustFeaturesForState
    {
        NSUInteger style = normalStyleMask;

        if (self.disabled)
            style &= ~NSResizableWindowMask;
        if (style != [self styleMask])
            [self setStyleMask:style];

        if (style & NSClosableWindowMask)
            [[self standardWindowButton:NSWindowCloseButton] setEnabled:!self.disabled];
        if (style & NSMiniaturizableWindowMask)
            [[self standardWindowButton:NSWindowMiniaturizeButton] setEnabled:!self.disabled];
    }

    - (void) setWindowFeatures:(const struct macdrv_window_features*)wf
    {
        normalStyleMask = style_mask_for_features(wf);
        [self adjustFeaturesForState];
        [self setHasShadow:wf->shadow];
    }

    - (void) setMacDrvState:(const struct macdrv_window_state*)state
    {
        NSInteger level;
        NSWindowCollectionBehavior behavior;

        self.disabled = state->disabled;
        self.noActivate = state->no_activate;

        self.floating = state->floating;
        level = state->floating ? NSFloatingWindowLevel : NSNormalWindowLevel;
        if (level != [self level])
            [self setLevel:level];

        behavior = NSWindowCollectionBehaviorDefault;
        if (state->excluded_by_expose)
            behavior |= NSWindowCollectionBehaviorTransient;
        else
            behavior |= NSWindowCollectionBehaviorManaged;
        if (state->excluded_by_cycle)
        {
            behavior |= NSWindowCollectionBehaviorIgnoresCycle;
            if ([self isVisible])
                [NSApp removeWindowsItem:self];
        }
        else
        {
            behavior |= NSWindowCollectionBehaviorParticipatesInCycle;
            if ([self isVisible])
                [NSApp addWindowsItem:self title:[self title] filename:NO];
        }
        [self setCollectionBehavior:behavior];
    }

    /* Returns whether or not the window was ordered in, which depends on if
       its frame intersects any screen. */
    - (BOOL) orderBelow:(WineWindow*)prev orAbove:(WineWindow*)next
    {
        BOOL on_screen = frame_intersects_screens([self frame], [NSScreen screens]);
        if (on_screen)
        {
            [NSApp transformProcessToForeground];

            if (prev)
                [self orderWindow:NSWindowBelow relativeTo:[prev windowNumber]];
            else
                [self orderWindow:NSWindowAbove relativeTo:[next windowNumber]];
            if (latentParentWindow)
            {
                [latentParentWindow addChildWindow:self ordered:NSWindowAbove];
                self.latentParentWindow = nil;
            }

            /* Cocoa may adjust the frame when the window is ordered onto the screen.
               Generate a frame-changed event just in case.  The back end will ignore
               it if nothing actually changed. */
            [self windowDidResize:nil];

            if (![self isExcludedFromWindowsMenu])
                [NSApp addWindowsItem:self title:[self title] filename:NO];
        }

        return on_screen;
    }

    - (void) doOrderOut
    {
        self.latentParentWindow = [self parentWindow];
        [latentParentWindow removeChildWindow:self];
        [self orderOut:nil];
        [NSApp removeWindowsItem:self];
    }

    - (BOOL) setFrameIfOnScreen:(NSRect)contentRect
    {
        NSArray* screens = [NSScreen screens];
        BOOL on_screen = [self isVisible];
        NSRect frame, oldFrame;

        if (![screens count]) return on_screen;

        /* Origin is (left, top) in a top-down space.  Need to convert it to
           (left, bottom) in a bottom-up space. */
        [[self class] flipRect:&contentRect];

        if (on_screen)
        {
            on_screen = frame_intersects_screens(contentRect, screens);
            if (!on_screen)
                [self doOrderOut];
        }

        oldFrame = [self frame];
        frame = [self frameRectForContentRect:contentRect];
        if (!NSEqualRects(frame, oldFrame))
        {
            if (NSEqualSizes(frame.size, oldFrame.size))
                [self setFrameOrigin:frame.origin];
            else
                [self setFrame:frame display:YES];
        }

        /* In case Cocoa adjusted the frame we tried to set, generate a frame-changed
           event.  The back end will ignore it if nothing actually changed. */
        [self windowDidResize:nil];

        return on_screen;
    }

    - (void) setMacDrvParentWindow:(WineWindow*)parent
    {
        if ([self parentWindow] != parent)
        {
            [[self parentWindow] removeChildWindow:self];
            self.latentParentWindow = nil;
            if ([self isVisible] && parent)
                [parent addChildWindow:self ordered:NSWindowAbove];
            else
                self.latentParentWindow = parent;
        }
    }

    - (void) setDisabled:(BOOL)newValue
    {
        if (disabled != newValue)
        {
            disabled = newValue;
            [self adjustFeaturesForState];
        }
    }

    - (BOOL) needsTransparency
    {
        return self.shape || self.colorKeyed || self.usePerPixelAlpha;
    }

    - (void) checkTransparency
    {
        if (![self isOpaque] && !self.needsTransparency)
        {
            [self setBackgroundColor:[NSColor windowBackgroundColor]];
            [self setOpaque:YES];
        }
        else if ([self isOpaque] && self.needsTransparency)
        {
            [self setBackgroundColor:[NSColor clearColor]];
            [self setOpaque:NO];
        }
    }

    - (void) setShape:(NSBezierPath*)newShape
    {
        if (shape == newShape) return;
        if (shape && newShape && [shape isEqual:newShape]) return;

        if (shape)
        {
            [[self contentView] setNeedsDisplayInRect:[shape bounds]];
            [shape release];
        }
        if (newShape)
            [[self contentView] setNeedsDisplayInRect:[newShape bounds]];

        shape = [newShape copy];
        self.shapeChangedSinceLastDraw = TRUE;

        [self checkTransparency];
    }

    - (void) postMouseButtonEvent:(NSEvent *)theEvent pressed:(int)pressed
    {
        CGPoint pt = CGEventGetLocation([theEvent CGEvent]);
        macdrv_event event;

        event.type = MOUSE_BUTTON;
        event.window = (macdrv_window)[self retain];
        event.mouse_button.button = [theEvent buttonNumber];
        event.mouse_button.pressed = pressed;
        event.mouse_button.x = pt.x;
        event.mouse_button.y = pt.y;
        event.mouse_button.time_ms = [NSApp ticksForEventTime:[theEvent timestamp]];

        [queue postEvent:&event];
    }

    - (void) makeFocused
    {
        NSArray* screens;

        [NSApp transformProcessToForeground];

        /* If a borderless window is offscreen, orderFront: won't move
           it onscreen like it would for a titled window.  Do that ourselves. */
        screens = [NSScreen screens];
        if (!([self styleMask] & NSTitledWindowMask) && ![self isVisible] &&
            !frame_intersects_screens([self frame], screens))
        {
            NSScreen* primaryScreen = [screens objectAtIndex:0];
            NSRect frame = [primaryScreen frame];
            [self setFrameTopLeftPoint:NSMakePoint(NSMinX(frame), NSMaxY(frame))];
            frame = [self constrainFrameRect:[self frame] toScreen:primaryScreen];
            [self setFrame:frame display:YES];
        }

        [self orderFront:nil];
        causing_becomeKeyWindow = TRUE;
        [self makeKeyWindow];
        causing_becomeKeyWindow = FALSE;
        if (latentParentWindow)
        {
            [latentParentWindow addChildWindow:self ordered:NSWindowAbove];
            self.latentParentWindow = nil;
        }
        if (![self isExcludedFromWindowsMenu])
            [NSApp addWindowsItem:self title:[self title] filename:NO];

        /* Cocoa may adjust the frame when the window is ordered onto the screen.
           Generate a frame-changed event just in case.  The back end will ignore
           it if nothing actually changed. */
        [self windowDidResize:nil];
    }


    /*
     * ---------- NSWindow method overrides ----------
     */
    - (BOOL) canBecomeKeyWindow
    {
        if (causing_becomeKeyWindow) return YES;
        if (self.disabled || self.noActivate) return NO;
        return [self isKeyWindow];
    }

    - (BOOL) canBecomeMainWindow
    {
        return [self canBecomeKeyWindow];
    }

    - (BOOL) isExcludedFromWindowsMenu
    {
        return !([self collectionBehavior] & NSWindowCollectionBehaviorParticipatesInCycle);
    }

    - (BOOL) validateMenuItem:(NSMenuItem *)menuItem
    {
        if ([menuItem action] == @selector(makeKeyAndOrderFront:))
            return [self isKeyWindow] || (!self.disabled && !self.noActivate);
        return [super validateMenuItem:menuItem];
    }

    /* We don't call this.  It's the action method of the items in the Window menu. */
    - (void) makeKeyAndOrderFront:(id)sender
    {
        if (![self isKeyWindow] && !self.disabled && !self.noActivate)
            [NSApp windowGotFocus:self];
    }

    - (void) sendEvent:(NSEvent*)event
    {
        if ([event type] == NSLeftMouseDown)
        {
            /* Since our windows generally claim they can't be made key, clicks
               in their title bars are swallowed by the theme frame stuff.  So,
               we hook directly into the event stream and assume that any click
               in the window will activate it, if Wine and the Win32 program
               accept. */
            if (![self isKeyWindow] && !self.disabled && !self.noActivate)
                [NSApp windowGotFocus:self];
        }

        [super sendEvent:event];
    }


    /*
     * ---------- NSResponder method overrides ----------
     */
    - (void) mouseDown:(NSEvent *)theEvent { [self postMouseButtonEvent:theEvent pressed:1]; }
    - (void) rightMouseDown:(NSEvent *)theEvent { [self mouseDown:theEvent]; }
    - (void) otherMouseDown:(NSEvent *)theEvent { [self mouseDown:theEvent]; }

    - (void) mouseUp:(NSEvent *)theEvent { [self postMouseButtonEvent:theEvent pressed:0]; }
    - (void) rightMouseUp:(NSEvent *)theEvent { [self mouseUp:theEvent]; }
    - (void) otherMouseUp:(NSEvent *)theEvent { [self mouseUp:theEvent]; }


    /*
     * ---------- NSWindowDelegate methods ----------
     */
    - (void)windowDidBecomeKey:(NSNotification *)notification
    {
        if (causing_becomeKeyWindow) return;

        [NSApp windowGotFocus:self];
    }

    - (void)windowDidMove:(NSNotification *)notification
    {
        [self windowDidResize:notification];
    }

    - (void)windowDidResignKey:(NSNotification *)notification
    {
        macdrv_event event;

        if (causing_becomeKeyWindow) return;

        event.type = WINDOW_LOST_FOCUS;
        event.window = (macdrv_window)[self retain];
        [queue postEvent:&event];
    }

    - (void)windowDidResize:(NSNotification *)notification
    {
        macdrv_event event;
        NSRect frame = [self contentRectForFrameRect:[self frame]];

        [[self class] flipRect:&frame];

        /* Coalesce events by discarding any previous ones still in the queue. */
        [queue discardEventsMatchingMask:event_mask_for_type(WINDOW_FRAME_CHANGED)
                               forWindow:self];

        event.type = WINDOW_FRAME_CHANGED;
        event.window = (macdrv_window)[self retain];
        event.window_frame_changed.frame = NSRectToCGRect(frame);
        [queue postEvent:&event];
    }

    - (BOOL)windowShouldClose:(id)sender
    {
        macdrv_event event;
        event.type = WINDOW_CLOSE_REQUESTED;
        event.window = (macdrv_window)[self retain];
        [queue postEvent:&event];
        return NO;
    }

@end


/***********************************************************************
 *              macdrv_create_cocoa_window
 *
 * Create a Cocoa window with the given content frame and features (e.g.
 * title bar, close box, etc.).
 */
macdrv_window macdrv_create_cocoa_window(const struct macdrv_window_features* wf,
        CGRect frame, void* hwnd, macdrv_event_queue queue)
{
    __block WineWindow* window;

    OnMainThread(^{
        window = [[WineWindow createWindowWithFeatures:wf
                                           windowFrame:NSRectFromCGRect(frame)
                                                  hwnd:hwnd
                                                 queue:(WineEventQueue*)queue] retain];
    });

    return (macdrv_window)window;
}

/***********************************************************************
 *              macdrv_destroy_cocoa_window
 *
 * Destroy a Cocoa window.
 */
void macdrv_destroy_cocoa_window(macdrv_window w)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineWindow* window = (WineWindow*)w;

    [window.queue discardEventsMatchingMask:-1 forWindow:window];
    [window close];
    [window release];

    [pool release];
}

/***********************************************************************
 *              macdrv_get_window_hwnd
 *
 * Get the hwnd that was set for the window at creation.
 */
void* macdrv_get_window_hwnd(macdrv_window w)
{
    WineWindow* window = (WineWindow*)w;
    return window.hwnd;
}

/***********************************************************************
 *              macdrv_set_cocoa_window_features
 *
 * Update a Cocoa window's features.
 */
void macdrv_set_cocoa_window_features(macdrv_window w,
        const struct macdrv_window_features* wf)
{
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        [window setWindowFeatures:wf];
    });
}

/***********************************************************************
 *              macdrv_set_cocoa_window_state
 *
 * Update a Cocoa window's state.
 */
void macdrv_set_cocoa_window_state(macdrv_window w,
        const struct macdrv_window_state* state)
{
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        [window setMacDrvState:state];
    });
}

/***********************************************************************
 *              macdrv_set_cocoa_window_title
 *
 * Set a Cocoa window's title.
 */
void macdrv_set_cocoa_window_title(macdrv_window w, const unsigned short* title,
        size_t length)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineWindow* window = (WineWindow*)w;
    NSString* titleString;

    if (title)
        titleString = [NSString stringWithCharacters:title length:length];
    else
        titleString = @"";
    OnMainThreadAsync(^{
        [window setTitle:titleString];
        if ([window isVisible] && ![window isExcludedFromWindowsMenu])
            [NSApp changeWindowsItem:window title:titleString filename:NO];
    });

    [pool release];
}

/***********************************************************************
 *              macdrv_order_cocoa_window
 *
 * Reorder a Cocoa window relative to other windows.  If prev is
 * non-NULL, it is ordered below that window.  Else, if next is non-NULL,
 * it is ordered above that window.  Otherwise, it is ordered to the
 * front.
 *
 * Returns true if the window has actually been ordered onto the screen
 * (i.e. if its frame intersects with a screen).  Otherwise, false.
 */
int macdrv_order_cocoa_window(macdrv_window w, macdrv_window prev,
        macdrv_window next)
{
    WineWindow* window = (WineWindow*)w;
    __block BOOL on_screen;

    OnMainThread(^{
        on_screen = [window orderBelow:(WineWindow*)prev
                               orAbove:(WineWindow*)next];
    });

    return on_screen;
}

/***********************************************************************
 *              macdrv_hide_cocoa_window
 *
 * Hides a Cocoa window.
 */
void macdrv_hide_cocoa_window(macdrv_window w)
{
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        [window doOrderOut];
    });
}

/***********************************************************************
 *              macdrv_set_cocoa_window_frame
 *
 * Move a Cocoa window.  If the window has been moved out of the bounds
 * of the desktop, it is ordered out.  (This routine won't ever order a
 * window in, though.)
 *
 * Returns true if the window is on screen; false otherwise.
 */
int macdrv_set_cocoa_window_frame(macdrv_window w, const CGRect* new_frame)
{
    WineWindow* window = (WineWindow*)w;
    __block BOOL on_screen;

    OnMainThread(^{
        on_screen = [window setFrameIfOnScreen:NSRectFromCGRect(*new_frame)];
    });

    return on_screen;
}

/***********************************************************************
 *              macdrv_get_cocoa_window_frame
 *
 * Gets the frame of a Cocoa window.
 */
void macdrv_get_cocoa_window_frame(macdrv_window w, CGRect* out_frame)
{
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        NSRect frame;

        frame = [window contentRectForFrameRect:[window frame]];
        [[window class] flipRect:&frame];
        *out_frame = NSRectToCGRect(frame);
    });
}

/***********************************************************************
 *              macdrv_set_cocoa_parent_window
 *
 * Sets the parent window for a Cocoa window.  If parent is NULL, clears
 * the parent window.
 */
void macdrv_set_cocoa_parent_window(macdrv_window w, macdrv_window parent)
{
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        [window setMacDrvParentWindow:(WineWindow*)parent];
    });
}

/***********************************************************************
 *              macdrv_set_window_surface
 */
void macdrv_set_window_surface(macdrv_window w, void *surface, pthread_mutex_t *mutex)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        window.surface = surface;
        window.surface_mutex = mutex;
    });

    [pool release];
}

/***********************************************************************
 *              macdrv_window_needs_display
 *
 * Mark a window as needing display in a specified rect (in non-client
 * area coordinates).
 */
void macdrv_window_needs_display(macdrv_window w, CGRect rect)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineWindow* window = (WineWindow*)w;

    OnMainThreadAsync(^{
        [[window contentView] setNeedsDisplayInRect:NSRectFromCGRect(rect)];
    });

    [pool release];
}

/***********************************************************************
 *              macdrv_set_window_shape
 *
 * Sets the shape of a Cocoa window from an array of rectangles.  If
 * rects is NULL, resets the window's shape to its frame.
 */
void macdrv_set_window_shape(macdrv_window w, const CGRect *rects, int count)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        if (!rects || !count)
            window.shape = nil;
        else
        {
            NSBezierPath* path;
            unsigned int i;

            path = [NSBezierPath bezierPath];
            for (i = 0; i < count; i++)
                [path appendBezierPathWithRect:NSRectFromCGRect(rects[i])];
            window.shape = path;
        }
    });

    [pool release];
}

/***********************************************************************
 *              macdrv_set_window_alpha
 */
void macdrv_set_window_alpha(macdrv_window w, CGFloat alpha)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineWindow* window = (WineWindow*)w;

    [window setAlphaValue:alpha];

    [pool release];
}

/***********************************************************************
 *              macdrv_set_window_color_key
 */
void macdrv_set_window_color_key(macdrv_window w, CGFloat keyRed, CGFloat keyGreen,
                                 CGFloat keyBlue)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        window.colorKeyed       = TRUE;
        window.colorKeyRed      = keyRed;
        window.colorKeyGreen    = keyGreen;
        window.colorKeyBlue     = keyBlue;
        [window checkTransparency];
    });

    [pool release];
}

/***********************************************************************
 *              macdrv_clear_window_color_key
 */
void macdrv_clear_window_color_key(macdrv_window w)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        window.colorKeyed = FALSE;
        [window checkTransparency];
    });

    [pool release];
}

/***********************************************************************
 *              macdrv_window_use_per_pixel_alpha
 */
void macdrv_window_use_per_pixel_alpha(macdrv_window w, int use_per_pixel_alpha)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        window.usePerPixelAlpha = use_per_pixel_alpha;
        [window checkTransparency];
    });

    [pool release];
}

/***********************************************************************
 *              macdrv_give_cocoa_window_focus
 *
 * Makes the Cocoa window "key" (gives it keyboard focus).  This also
 * orders it front and, if its frame was not within the desktop bounds,
 * Cocoa will typically move it on-screen.
 */
void macdrv_give_cocoa_window_focus(macdrv_window w)
{
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        [window makeFocused];
    });
}
