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

#import <Carbon/Carbon.h>

#import "cocoa_window.h"

#include "macdrv_cocoa.h"
#import "cocoa_app.h"
#import "cocoa_event.h"
#import "cocoa_opengl.h"


#if !defined(MAC_OS_X_VERSION_10_7) || MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_7
enum {
    NSWindowCollectionBehaviorFullScreenPrimary = 1 << 7,
    NSWindowCollectionBehaviorFullScreenAuxiliary = 1 << 8,
    NSWindowFullScreenButton = 7,
    NSFullScreenWindowMask = 1 << 14,
};

@interface NSWindow (WineFullScreenExtensions)
    - (void) toggleFullScreen:(id)sender;
@end
#endif


/* Additional Mac virtual keycode, to complement those in Carbon's <HIToolbox/Events.h>. */
enum {
    kVK_RightCommand              = 0x36, /* Invented for Wine; was unused */
};


static NSUInteger style_mask_for_features(const struct macdrv_window_features* wf)
{
    NSUInteger style_mask;

    if (wf->title_bar)
    {
        style_mask = NSTitledWindowMask;
        if (wf->close_button) style_mask |= NSClosableWindowMask;
        if (wf->minimize_button) style_mask |= NSMiniaturizableWindowMask;
        if (wf->resizable || wf->maximize_button) style_mask |= NSResizableWindowMask;
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


static NSScreen* screen_covered_by_rect(NSRect rect, NSArray* screens)
{
    for (NSScreen* screen in screens)
    {
        if (NSContainsRect(rect, [screen frame]))
            return screen;
    }
    return nil;
}


/* We rely on the supposedly device-dependent modifier flags to distinguish the
   keys on the left side of the keyboard from those on the right.  Some event
   sources don't set those device-depdendent flags.  If we see a device-independent
   flag for a modifier without either corresponding device-dependent flag, assume
   the left one. */
static inline void fix_device_modifiers_by_generic(NSUInteger* modifiers)
{
    if ((*modifiers & (NX_COMMANDMASK | NX_DEVICELCMDKEYMASK | NX_DEVICERCMDKEYMASK)) == NX_COMMANDMASK)
        *modifiers |= NX_DEVICELCMDKEYMASK;
    if ((*modifiers & (NX_SHIFTMASK | NX_DEVICELSHIFTKEYMASK | NX_DEVICERSHIFTKEYMASK)) == NX_SHIFTMASK)
        *modifiers |= NX_DEVICELSHIFTKEYMASK;
    if ((*modifiers & (NX_CONTROLMASK | NX_DEVICELCTLKEYMASK | NX_DEVICERCTLKEYMASK)) == NX_CONTROLMASK)
        *modifiers |= NX_DEVICELCTLKEYMASK;
    if ((*modifiers & (NX_ALTERNATEMASK | NX_DEVICELALTKEYMASK | NX_DEVICERALTKEYMASK)) == NX_ALTERNATEMASK)
        *modifiers |= NX_DEVICELALTKEYMASK;
}

/* As we manipulate individual bits of a modifier mask, we can end up with
   inconsistent sets of flags.  In particular, we might set or clear one of the
   left/right-specific bits, but not the corresponding non-side-specific bit.
   Fix that.  If either side-specific bit is set, set the non-side-specific bit,
   otherwise clear it. */
static inline void fix_generic_modifiers_by_device(NSUInteger* modifiers)
{
    if (*modifiers & (NX_DEVICELCMDKEYMASK | NX_DEVICERCMDKEYMASK))
        *modifiers |= NX_COMMANDMASK;
    else
        *modifiers &= ~NX_COMMANDMASK;
    if (*modifiers & (NX_DEVICELSHIFTKEYMASK | NX_DEVICERSHIFTKEYMASK))
        *modifiers |= NX_SHIFTMASK;
    else
        *modifiers &= ~NX_SHIFTMASK;
    if (*modifiers & (NX_DEVICELCTLKEYMASK | NX_DEVICERCTLKEYMASK))
        *modifiers |= NX_CONTROLMASK;
    else
        *modifiers &= ~NX_CONTROLMASK;
    if (*modifiers & (NX_DEVICELALTKEYMASK | NX_DEVICERALTKEYMASK))
        *modifiers |= NX_ALTERNATEMASK;
    else
        *modifiers &= ~NX_ALTERNATEMASK;
}

static inline NSUInteger adjusted_modifiers_for_option_behavior(NSUInteger modifiers)
{
    fix_device_modifiers_by_generic(&modifiers);
    if (left_option_is_alt && (modifiers & NX_DEVICELALTKEYMASK))
    {
        modifiers |= NX_DEVICELCMDKEYMASK;
        modifiers &= ~NX_DEVICELALTKEYMASK;
    }
    if (right_option_is_alt && (modifiers & NX_DEVICERALTKEYMASK))
    {
        modifiers |= NX_DEVICERCMDKEYMASK;
        modifiers &= ~NX_DEVICERALTKEYMASK;
    }
    fix_generic_modifiers_by_device(&modifiers);

    return modifiers;
}


@interface WineContentView : NSView <NSTextInputClient>
{
    NSMutableArray* glContexts;
    NSMutableArray* pendingGlContexts;
    BOOL clearedGlSurface;

    NSMutableAttributedString* markedText;
    NSRange markedTextSelection;
}

    - (void) addGLContext:(WineOpenGLContext*)context;
    - (void) removeGLContext:(WineOpenGLContext*)context;
    - (void) updateGLContexts;

@end


@interface WineWindow ()

@property (readwrite, nonatomic) BOOL disabled;
@property (readwrite, nonatomic) BOOL noActivate;
@property (readwrite, nonatomic) BOOL floating;
@property (readwrite, getter=isFakingClose, nonatomic) BOOL fakingClose;
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

@property (assign, nonatomic) void* imeData;
@property (nonatomic) BOOL commandDone;

@property (retain, nonatomic) NSTimer* liveResizeDisplayTimer;

    - (void) updateColorSpace;

    - (BOOL) becameEligibleParentOrChild;
    - (void) becameIneligibleChild;

@end


@implementation WineContentView

    - (void) dealloc
    {
        [markedText release];
        [glContexts release];
        [pendingGlContexts release];
        [super dealloc];
    }

    - (BOOL) isFlipped
    {
        return YES;
    }

    - (void) drawRect:(NSRect)rect
    {
        WineWindow* window = (WineWindow*)[self window];

        for (WineOpenGLContext* context in pendingGlContexts)
        {
            if (!clearedGlSurface)
            {
                context.shouldClearToBlack = TRUE;
                clearedGlSurface = TRUE;
            }
            context.needsUpdate = TRUE;
        }
        [glContexts addObjectsFromArray:pendingGlContexts];
        [pendingGlContexts removeAllObjects];

        if ([window contentView] != self)
            return;

        if (window.surface && window.surface_mutex &&
            !pthread_mutex_lock(window.surface_mutex))
        {
            const CGRect* rects;
            int count;

            if (get_surface_blit_rects(window.surface, &rects, &count) && count)
            {
                CGContextRef context;
                int i;

                [window.shape addClip];

                context = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
                CGContextSetBlendMode(context, kCGBlendModeCopy);

                for (i = 0; i < count; i++)
                {
                    CGRect imageRect;
                    CGImageRef image;

                    imageRect = CGRectIntersection(rects[i], NSRectToCGRect(rect));
                    image = create_surface_image(window.surface, &imageRect, FALSE);

                    if (image)
                    {
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

                        CGContextDrawImage(context, imageRect, image);

                        CGImageRelease(image);
                    }
                }
            }

            pthread_mutex_unlock(window.surface_mutex);
        }

        // If the window may be transparent, then we have to invalidate the
        // shadow every time we draw.  Also, if this is the first time we've
        // drawn since changing from transparent to opaque.
        if (![window isOpaque] || window.shapeChangedSinceLastDraw)
        {
            window.shapeChangedSinceLastDraw = FALSE;
            [window invalidateShadow];
        }
    }

    - (void) addGLContext:(WineOpenGLContext*)context
    {
        if (!glContexts)
            glContexts = [[NSMutableArray alloc] init];
        if (!pendingGlContexts)
            pendingGlContexts = [[NSMutableArray alloc] init];

        if ([[self window] windowNumber] > 0 && !NSIsEmptyRect([self visibleRect]))
        {
            [glContexts addObject:context];
            if (!clearedGlSurface)
            {
                context.shouldClearToBlack = TRUE;
                clearedGlSurface = TRUE;
            }
            context.needsUpdate = TRUE;
        }
        else
        {
            [pendingGlContexts addObject:context];
            [self setNeedsDisplay:YES];
        }

        [(WineWindow*)[self window] updateColorSpace];
    }

    - (void) removeGLContext:(WineOpenGLContext*)context
    {
        [glContexts removeObjectIdenticalTo:context];
        [pendingGlContexts removeObjectIdenticalTo:context];
        [(WineWindow*)[self window] updateColorSpace];
    }

    - (void) updateGLContexts
    {
        for (WineOpenGLContext* context in glContexts)
            context.needsUpdate = TRUE;
    }

    - (BOOL) hasGLContext
    {
        return [glContexts count] || [pendingGlContexts count];
    }

    - (BOOL) acceptsFirstMouse:(NSEvent*)theEvent
    {
        return YES;
    }

    - (BOOL) preservesContentDuringLiveResize
    {
        // Returning YES from this tells Cocoa to keep our view's content during
        // a Cocoa-driven resize.  In theory, we're also supposed to override
        // -setFrameSize: to mark exposed sections as needing redisplay, but
        // user32 will take care of that in a roundabout way.  This way, we don't
        // redraw until the window surface is flushed.
        //
        // This doesn't do anything when we resize the window ourselves.
        return YES;
    }

    - (BOOL)acceptsFirstResponder
    {
        return [[self window] contentView] == self;
    }

    - (void) completeText:(NSString*)text
    {
        macdrv_event* event;
        WineWindow* window = (WineWindow*)[self window];

        event = macdrv_create_event(IM_SET_TEXT, window);
        event->im_set_text.data = [window imeData];
        event->im_set_text.text = (CFStringRef)[text copy];
        event->im_set_text.complete = TRUE;

        [[window queue] postEvent:event];

        macdrv_release_event(event);

        [markedText deleteCharactersInRange:NSMakeRange(0, [markedText length])];
        markedTextSelection = NSMakeRange(0, 0);
        [[self inputContext] discardMarkedText];
    }

    - (NSFocusRingType) focusRingType
    {
        return NSFocusRingTypeNone;
    }

    /*
     * ---------- NSTextInputClient methods ----------
     */
    - (NSTextInputContext*) inputContext
    {
        if (!markedText)
            markedText = [[NSMutableAttributedString alloc] init];
        return [super inputContext];
    }

    - (void) insertText:(id)string replacementRange:(NSRange)replacementRange
    {
        if ([string isKindOfClass:[NSAttributedString class]])
            string = [string string];

        if ([string isKindOfClass:[NSString class]])
            [self completeText:string];
    }

    - (void) doCommandBySelector:(SEL)aSelector
    {
        [(WineWindow*)[self window] setCommandDone:TRUE];
    }

    - (void) setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange
    {
        if ([string isKindOfClass:[NSAttributedString class]])
            string = [string string];

        if ([string isKindOfClass:[NSString class]])
        {
            macdrv_event* event;
            WineWindow* window = (WineWindow*)[self window];

            if (replacementRange.location == NSNotFound)
                replacementRange = NSMakeRange(0, [markedText length]);

            [markedText replaceCharactersInRange:replacementRange withString:string];
            markedTextSelection = selectedRange;
            markedTextSelection.location += replacementRange.location;

            event = macdrv_create_event(IM_SET_TEXT, window);
            event->im_set_text.data = [window imeData];
            event->im_set_text.text = (CFStringRef)[[markedText string] copy];
            event->im_set_text.complete = FALSE;
            event->im_set_text.cursor_pos = markedTextSelection.location;

            [[window queue] postEvent:event];

            macdrv_release_event(event);

            [[self inputContext] invalidateCharacterCoordinates];
        }
    }

    - (void) unmarkText
    {
        [self completeText:nil];
    }

    - (NSRange) selectedRange
    {
        return markedTextSelection;
    }

    - (NSRange) markedRange
    {
        NSRange range = NSMakeRange(0, [markedText length]);
        if (!range.length)
            range.location = NSNotFound;
        return range;
    }

    - (BOOL) hasMarkedText
    {
        return [markedText length] > 0;
    }

    - (NSAttributedString*) attributedSubstringForProposedRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange
    {
        if (aRange.location >= [markedText length])
            return nil;

        aRange = NSIntersectionRange(aRange, NSMakeRange(0, [markedText length]));
        if (actualRange)
            *actualRange = aRange;
        return [markedText attributedSubstringFromRange:aRange];
    }

    - (NSArray*) validAttributesForMarkedText
    {
        return [NSArray array];
    }

    - (NSRect) firstRectForCharacterRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange
    {
        macdrv_query* query;
        WineWindow* window = (WineWindow*)[self window];
        NSRect ret;

        aRange = NSIntersectionRange(aRange, NSMakeRange(0, [markedText length]));

        query = macdrv_create_query();
        query->type = QUERY_IME_CHAR_RECT;
        query->window = (macdrv_window)[window retain];
        query->ime_char_rect.data = [window imeData];
        query->ime_char_rect.range = CFRangeMake(aRange.location, aRange.length);

        if ([window.queue query:query timeout:1])
        {
            aRange = NSMakeRange(query->ime_char_rect.range.location, query->ime_char_rect.range.length);
            ret = NSRectFromCGRect(query->ime_char_rect.rect);
            [[WineApplicationController sharedController] flipRect:&ret];
        }
        else
            ret = NSMakeRect(100, 100, aRange.length ? 1 : 0, 12);

        macdrv_release_query(query);

        if (actualRange)
            *actualRange = aRange;
        return ret;
    }

    - (NSUInteger) characterIndexForPoint:(NSPoint)aPoint
    {
        return NSNotFound;
    }

    - (NSInteger) windowLevel
    {
        return [[self window] level];
    }

@end


@implementation WineWindow

    static WineWindow* causing_becomeKeyWindow;

    @synthesize disabled, noActivate, floating, fullscreen, fakingClose, latentParentWindow, hwnd, queue;
    @synthesize surface, surface_mutex;
    @synthesize shape, shapeChangedSinceLastDraw;
    @synthesize colorKeyed, colorKeyRed, colorKeyGreen, colorKeyBlue;
    @synthesize usePerPixelAlpha;
    @synthesize imeData, commandDone;
    @synthesize liveResizeDisplayTimer;

    + (WineWindow*) createWindowWithFeatures:(const struct macdrv_window_features*)wf
                                 windowFrame:(NSRect)window_frame
                                        hwnd:(void*)hwnd
                                       queue:(WineEventQueue*)queue
    {
        WineWindow* window;
        WineContentView* contentView;
        NSTrackingArea* trackingArea;
        NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];

        [[WineApplicationController sharedController] flipRect:&window_frame];

        window = [[[self alloc] initWithContentRect:window_frame
                                          styleMask:style_mask_for_features(wf)
                                            backing:NSBackingStoreBuffered
                                              defer:YES] autorelease];

        if (!window) return nil;

        /* Standardize windows to eliminate differences between titled and
           borderless windows and between NSWindow and NSPanel. */
        [window setHidesOnDeactivate:NO];
        [window setReleasedWhenClosed:NO];

        [window setOneShot:YES];
        [window disableCursorRects];
        [window setShowsResizeIndicator:NO];
        [window setHasShadow:wf->shadow];
        [window setAcceptsMouseMovedEvents:YES];
        [window setColorSpace:[NSColorSpace genericRGBColorSpace]];
        [window setDelegate:window];
        window.hwnd = hwnd;
        window.queue = queue;
        window->savedContentMinSize = NSZeroSize;
        window->savedContentMaxSize = NSMakeSize(FLT_MAX, FLT_MAX);
        window->resizable = wf->resizable;

        [window registerForDraggedTypes:[NSArray arrayWithObjects:(NSString*)kUTTypeData,
                                                                  (NSString*)kUTTypeContent,
                                                                  nil]];

        contentView = [[[WineContentView alloc] initWithFrame:NSZeroRect] autorelease];
        if (!contentView)
            return nil;
        [contentView setAutoresizesSubviews:NO];

        /* We use tracking areas in addition to setAcceptsMouseMovedEvents:YES
           because they give us mouse moves in the background. */
        trackingArea = [[[NSTrackingArea alloc] initWithRect:[contentView bounds]
                                                     options:(NSTrackingMouseMoved |
                                                              NSTrackingActiveAlways |
                                                              NSTrackingInVisibleRect)
                                                       owner:window
                                                    userInfo:nil] autorelease];
        if (!trackingArea)
            return nil;
        [contentView addTrackingArea:trackingArea];

        [window setContentView:contentView];
        [window setInitialFirstResponder:contentView];

        [nc addObserver:window
               selector:@selector(updateFullscreen)
                   name:NSApplicationDidChangeScreenParametersNotification
                 object:NSApp];
        [window updateFullscreen];

        [nc addObserver:window
               selector:@selector(applicationWillHide)
                   name:NSApplicationWillHideNotification
                 object:NSApp];
        [nc addObserver:window
               selector:@selector(applicationDidUnhide)
                   name:NSApplicationDidUnhideNotification
                 object:NSApp];

        return window;
    }

    - (void) dealloc
    {
        [[NSNotificationCenter defaultCenter] removeObserver:self];
        [liveResizeDisplayTimer invalidate];
        [liveResizeDisplayTimer release];
        [queue release];
        [latentChildWindows release];
        [latentParentWindow release];
        [shape release];
        [super dealloc];
    }

    - (BOOL) preventResizing
    {
        BOOL preventForClipping = cursor_clipping_locks_windows && [[WineApplicationController sharedController] clippingCursor];
        return ([self styleMask] & NSResizableWindowMask) && (disabled || !resizable || maximized || preventForClipping);
    }

    - (void) adjustFeaturesForState
    {
        NSUInteger style = [self styleMask];

        if (style & NSClosableWindowMask)
            [[self standardWindowButton:NSWindowCloseButton] setEnabled:!self.disabled];
        if (style & NSMiniaturizableWindowMask)
            [[self standardWindowButton:NSWindowMiniaturizeButton] setEnabled:!self.disabled];
        if (style & NSResizableWindowMask)
            [[self standardWindowButton:NSWindowZoomButton] setEnabled:!self.disabled];
        if ([self respondsToSelector:@selector(toggleFullScreen:)])
        {
            if ([self collectionBehavior] & NSWindowCollectionBehaviorFullScreenPrimary)
                [[self standardWindowButton:NSWindowFullScreenButton] setEnabled:!self.disabled];
        }

        if ([self preventResizing])
        {
            NSSize size = [self contentRectForFrameRect:[self frame]].size;
            [self setContentMinSize:size];
            [self setContentMaxSize:size];
        }
        else
        {
            [self setContentMaxSize:savedContentMaxSize];
            [self setContentMinSize:savedContentMinSize];
        }

        if (allow_immovable_windows || cursor_clipping_locks_windows)
        {
            if (allow_immovable_windows && (disabled || maximized))
                [self setMovable:NO];
            else if (cursor_clipping_locks_windows && [[WineApplicationController sharedController] clippingCursor])
                [self setMovable:NO];
            else
                [self setMovable:YES];
        }
    }

    - (void) adjustFullScreenBehavior:(NSWindowCollectionBehavior)behavior
    {
        if ([self respondsToSelector:@selector(toggleFullScreen:)])
        {
            NSUInteger style = [self styleMask];

            if (behavior & NSWindowCollectionBehaviorParticipatesInCycle &&
                style & NSResizableWindowMask && !(style & NSUtilityWindowMask))
            {
                behavior |= NSWindowCollectionBehaviorFullScreenPrimary;
                behavior &= ~NSWindowCollectionBehaviorFullScreenAuxiliary;
            }
            else
            {
                behavior &= ~NSWindowCollectionBehaviorFullScreenPrimary;
                behavior |= NSWindowCollectionBehaviorFullScreenAuxiliary;
                if (style & NSFullScreenWindowMask)
                    [self toggleFullScreen:nil];
            }
        }

        if (behavior != [self collectionBehavior])
        {
            [self setCollectionBehavior:behavior];
            [self adjustFeaturesForState];
        }
    }

    - (void) setWindowFeatures:(const struct macdrv_window_features*)wf
    {
        static const NSUInteger usedStyles = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask |
                                             NSResizableWindowMask | NSUtilityWindowMask | NSBorderlessWindowMask;
        NSUInteger currentStyle = [self styleMask];
        NSUInteger newStyle = style_mask_for_features(wf) | (currentStyle & ~usedStyles);

        if (newStyle != currentStyle)
        {
            NSString* title = [[[self title] copy] autorelease];
            BOOL showingButtons = (currentStyle & (NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask)) != 0;
            BOOL shouldShowButtons = (newStyle & (NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask)) != 0;
            if (shouldShowButtons != showingButtons && !((newStyle ^ currentStyle) & NSClosableWindowMask))
            {
                // -setStyleMask: is buggy on 10.7+ with respect to NSResizableWindowMask.
                // If transitioning from NSTitledWindowMask | NSResizableWindowMask to
                // just NSTitledWindowMask, the window buttons should disappear rather
                // than just being disabled.  But they don't.  Similarly in reverse.
                // The workaround is to also toggle NSClosableWindowMask at the same time.
                [self setStyleMask:newStyle ^ NSClosableWindowMask];
            }
            [self setStyleMask:newStyle];

            // -setStyleMask: resets the firstResponder to the window.  Set it
            // back to the content view.
            if ([[self contentView] acceptsFirstResponder])
                [self makeFirstResponder:[self contentView]];

            [self adjustFullScreenBehavior:[self collectionBehavior]];

            if ([[self title] length] == 0 && [title length] > 0)
                [self setTitle:title];
        }

        resizable = wf->resizable;
        [self adjustFeaturesForState];
        [self setHasShadow:wf->shadow];
    }

    // Indicates if the window would be visible if the app were not hidden.
    - (BOOL) wouldBeVisible
    {
        return [NSApp isHidden] ? savedVisibleState : [self isVisible];
    }

    - (BOOL) isOrderedIn
    {
        return [self wouldBeVisible] || [self isMiniaturized];
    }

    - (NSInteger) minimumLevelForActive:(BOOL)active
    {
        NSInteger level;

        if (self.floating && (active || topmost_float_inactive == TOPMOST_FLOAT_INACTIVE_ALL ||
                              (topmost_float_inactive == TOPMOST_FLOAT_INACTIVE_NONFULLSCREEN && !fullscreen)))
            level = NSFloatingWindowLevel;
        else
            level = NSNormalWindowLevel;

        if (active)
        {
            BOOL captured;

            captured = (fullscreen || [self screen]) && [[WineApplicationController sharedController] areDisplaysCaptured];

            if (captured || fullscreen)
            {
                if (captured)
                    level = CGShieldingWindowLevel() + 1; /* Need +1 or we don't get mouse moves */
                else
                    level = NSMainMenuWindowLevel + 1;

                if (self.floating)
                    level++;
            }
        }

        return level;
    }

    - (void) postDidUnminimizeEvent
    {
        macdrv_event* event;

        /* Coalesce events by discarding any previous ones still in the queue. */
        [queue discardEventsMatchingMask:event_mask_for_type(WINDOW_DID_UNMINIMIZE)
                               forWindow:self];

        event = macdrv_create_event(WINDOW_DID_UNMINIMIZE, self);
        [queue postEvent:event];
        macdrv_release_event(event);
    }

    - (void) setMacDrvState:(const struct macdrv_window_state*)state
    {
        NSWindowCollectionBehavior behavior;

        self.disabled = state->disabled;
        self.noActivate = state->no_activate;

        if (self.floating != state->floating)
        {
            self.floating = state->floating;
            if (state->floating)
            {
                // Became floating.  If child of non-floating window, make that
                // relationship latent.
                WineWindow* parent = (WineWindow*)[self parentWindow];
                if (parent && !parent.floating)
                    [self becameIneligibleChild];
            }
            else
            {
                // Became non-floating.  If parent of floating children, make that
                // relationship latent.
                WineWindow* child;
                for (child in [[[self childWindows] copy] autorelease])
                {
                    if (child.floating)
                        [child becameIneligibleChild];
                }
            }

            // Check our latent relationships.  If floating status was the only
            // reason they were latent, then make them active.
            if ([self isVisible])
                [self becameEligibleParentOrChild];

            [[WineApplicationController sharedController] adjustWindowLevels];
        }

        behavior = NSWindowCollectionBehaviorDefault;
        if (state->excluded_by_expose)
            behavior |= NSWindowCollectionBehaviorTransient;
        else
            behavior |= NSWindowCollectionBehaviorManaged;
        if (state->excluded_by_cycle)
        {
            behavior |= NSWindowCollectionBehaviorIgnoresCycle;
            if ([self isOrderedIn])
                [NSApp removeWindowsItem:self];
        }
        else
        {
            behavior |= NSWindowCollectionBehaviorParticipatesInCycle;
            if ([self isOrderedIn])
                [NSApp addWindowsItem:self title:[self title] filename:NO];
        }
        [self adjustFullScreenBehavior:behavior];

        if (state->minimized_valid)
        {
            macdrv_event_mask discard = event_mask_for_type(WINDOW_DID_UNMINIMIZE);

            pendingMinimize = FALSE;
            if (state->minimized && ![self isMiniaturized])
            {
                if ([self wouldBeVisible])
                {
                    if ([self styleMask] & NSFullScreenWindowMask)
                    {
                        [self postDidUnminimizeEvent];
                        discard &= ~event_mask_for_type(WINDOW_DID_UNMINIMIZE);
                    }
                    else
                    {
                        [super miniaturize:nil];
                        discard |= event_mask_for_type(WINDOW_BROUGHT_FORWARD) |
                                   event_mask_for_type(WINDOW_GOT_FOCUS) |
                                   event_mask_for_type(WINDOW_LOST_FOCUS);
                    }
                }
                else
                    pendingMinimize = TRUE;
            }
            else if (!state->minimized && [self isMiniaturized])
            {
                ignore_windowDeminiaturize = TRUE;
                [self deminiaturize:nil];
                discard |= event_mask_for_type(WINDOW_LOST_FOCUS);
            }

            if (discard)
                [queue discardEventsMatchingMask:discard forWindow:self];
        }

        if (state->maximized != maximized)
        {
            maximized = state->maximized;
            [self adjustFeaturesForState];
        }
    }

    - (BOOL) addChildWineWindow:(WineWindow*)child assumeVisible:(BOOL)assumeVisible
    {
        BOOL reordered = FALSE;

        if ([self isVisible] && (assumeVisible || [child isVisible]) && (self.floating || !child.floating))
        {
            if ([self level] > [child level])
                [child setLevel:[self level]];
            [self addChildWindow:child ordered:NSWindowAbove];
            [latentChildWindows removeObjectIdenticalTo:child];
            child.latentParentWindow = nil;
            reordered = TRUE;
        }
        else
        {
            if (!latentChildWindows)
                latentChildWindows = [[NSMutableArray alloc] init];
            if (![latentChildWindows containsObject:child])
                [latentChildWindows addObject:child];
            child.latentParentWindow = self;
        }

        return reordered;
    }

    - (BOOL) addChildWineWindow:(WineWindow*)child
    {
        return [self addChildWineWindow:child assumeVisible:FALSE];
    }

    - (void) removeChildWineWindow:(WineWindow*)child
    {
        [self removeChildWindow:child];
        if (child.latentParentWindow == self)
            child.latentParentWindow = nil;
        [latentChildWindows removeObjectIdenticalTo:child];
    }

    - (BOOL) becameEligibleParentOrChild
    {
        BOOL reordered = FALSE;
        NSUInteger count;

        if (latentParentWindow.floating || !self.floating)
        {
            // If we aren't visible currently, we assume that we should be and soon
            // will be.  So, if the latent parent is visible that's enough to assume
            // we can establish the parent-child relationship in Cocoa.  That will
            // actually make us visible, which is fine.
            if ([latentParentWindow addChildWineWindow:self assumeVisible:TRUE])
                reordered = TRUE;
        }

        // Here, though, we may not actually be visible yet and adding a child
        // won't make us visible.  The caller will have to call this method
        // again after actually making us visible.
        if ([self isVisible] && (count = [latentChildWindows count]))
        {
            NSMutableIndexSet* indexesToRemove = [NSMutableIndexSet indexSet];
            NSUInteger i;

            for (i = 0; i < count; i++)
            {
                WineWindow* child = [latentChildWindows objectAtIndex:i];
                if ([child isVisible] && (self.floating || !child.floating))
                {
                    if (child.latentParentWindow == self)
                    {
                        if ([self level] > [child level])
                            [child setLevel:[self level]];
                        [self addChildWindow:child ordered:NSWindowAbove];
                        child.latentParentWindow = nil;
                        reordered = TRUE;
                    }
                    else
                        ERR(@"shouldn't happen: %@ thinks %@ is a latent child, but it doesn't agree\n", self, child);
                    [indexesToRemove addIndex:i];
                }
            }

            [latentChildWindows removeObjectsAtIndexes:indexesToRemove];
        }

        return reordered;
    }

    - (void) becameIneligibleChild
    {
        WineWindow* parent = (WineWindow*)[self parentWindow];
        if (parent)
        {
            if (!parent->latentChildWindows)
                parent->latentChildWindows = [[NSMutableArray alloc] init];
            [parent->latentChildWindows insertObject:self atIndex:0];
            self.latentParentWindow = parent;
            [parent removeChildWindow:self];
        }
    }

    - (void) becameIneligibleParentOrChild
    {
        NSArray* childWindows = [self childWindows];

        [self becameIneligibleChild];

        if ([childWindows count])
        {
            WineWindow* child;

            childWindows = [[childWindows copy] autorelease];
            for (child in childWindows)
            {
                child.latentParentWindow = self;
                [self removeChildWindow:child];
            }

            if (latentChildWindows)
                [latentChildWindows replaceObjectsInRange:NSMakeRange(0, 0) withObjectsFromArray:childWindows];
            else
                latentChildWindows = [childWindows mutableCopy];
        }
    }

    // Determine if, among Wine windows, this window is directly above or below
    // a given other Wine window with no other Wine window intervening.
    // Intervening non-Wine windows are ignored.
    - (BOOL) isOrdered:(NSWindowOrderingMode)orderingMode relativeTo:(WineWindow*)otherWindow
    {
        NSNumber* windowNumber;
        NSNumber* otherWindowNumber;
        NSArray* windowNumbers;
        NSUInteger windowIndex, otherWindowIndex, lowIndex, highIndex, i;

        if (![self isVisible] || ![otherWindow isVisible])
            return FALSE;

        windowNumber = [NSNumber numberWithInteger:[self windowNumber]];
        otherWindowNumber = [NSNumber numberWithInteger:[otherWindow windowNumber]];
        windowNumbers = [[self class] windowNumbersWithOptions:0];
        windowIndex = [windowNumbers indexOfObject:windowNumber];
        otherWindowIndex = [windowNumbers indexOfObject:otherWindowNumber];

        if (windowIndex == NSNotFound || otherWindowIndex == NSNotFound)
            return FALSE;

        if (orderingMode == NSWindowAbove)
        {
            lowIndex = windowIndex;
            highIndex = otherWindowIndex;
        }
        else if (orderingMode == NSWindowBelow)
        {
            lowIndex = otherWindowIndex;
            highIndex = windowIndex;
        }
        else
            return FALSE;

        if (highIndex <= lowIndex)
            return FALSE;

        for (i = lowIndex + 1; i < highIndex; i++)
        {
            NSInteger interveningWindowNumber = [[windowNumbers objectAtIndex:i] integerValue];
            NSWindow* interveningWindow = [NSApp windowWithWindowNumber:interveningWindowNumber];
            if ([interveningWindow isKindOfClass:[WineWindow class]])
                return FALSE;
        }

        return TRUE;
    }

    - (void) order:(NSWindowOrderingMode)mode childWindow:(WineWindow*)child relativeTo:(WineWindow*)other
    {
        NSMutableArray* windowNumbers;
        NSNumber* childWindowNumber;
        NSUInteger otherIndex, limit;
        NSArray* origChildren;
        NSMutableArray* children;

        // Get the z-order from the window server and modify it to reflect the
        // requested window ordering.
        windowNumbers = [[[[self class] windowNumbersWithOptions:NSWindowNumberListAllSpaces] mutableCopy] autorelease];
        childWindowNumber = [NSNumber numberWithInteger:[child windowNumber]];
        [windowNumbers removeObject:childWindowNumber];
        otherIndex = [windowNumbers indexOfObject:[NSNumber numberWithInteger:[other windowNumber]]];
        [windowNumbers insertObject:childWindowNumber atIndex:otherIndex + (mode == NSWindowAbove ? 0 : 1)];

        // Get our child windows and sort them in the reverse of the desired
        // z-order (back-to-front).
        origChildren = [self childWindows];
        children = [[origChildren mutableCopy] autorelease];
        [children sortWithOptions:NSSortStable
                  usingComparator:^NSComparisonResult(id obj1, id obj2){
            NSNumber* window1Number = [NSNumber numberWithInteger:[obj1 windowNumber]];
            NSNumber* window2Number = [NSNumber numberWithInteger:[obj2 windowNumber]];
            NSUInteger index1 = [windowNumbers indexOfObject:window1Number];
            NSUInteger index2 = [windowNumbers indexOfObject:window2Number];
            if (index1 == NSNotFound)
            {
                if (index2 == NSNotFound)
                    return NSOrderedSame;
                else
                    return NSOrderedAscending;
            }
            else if (index2 == NSNotFound)
                return NSOrderedDescending;
            else if (index1 < index2)
                return NSOrderedDescending;
            else if (index2 < index1)
                return NSOrderedAscending;

            return NSOrderedSame;
        }];

        // If the current and desired children arrays match up to a point, leave
        // those matching children alone.
        limit = MIN([origChildren count], [children count]);
        for (otherIndex = 0; otherIndex < limit; otherIndex++)
        {
            if ([origChildren objectAtIndex:otherIndex] != [children objectAtIndex:otherIndex])
                break;
        }
        [children removeObjectsInRange:NSMakeRange(0, otherIndex)];

        // Remove all of the child windows and re-add them back-to-front so they
        // are in the desired order.
        for (other in children)
            [self removeChildWindow:other];
        for (other in children)
            [self addChildWindow:other ordered:NSWindowAbove];
    }

    /* Returns whether or not the window was ordered in, which depends on if
       its frame intersects any screen. */
    - (void) orderBelow:(WineWindow*)prev orAbove:(WineWindow*)next activate:(BOOL)activate
    {
        WineApplicationController* controller = [WineApplicationController sharedController];
        if (![self isMiniaturized])
        {
            BOOL needAdjustWindowLevels = FALSE;
            BOOL wasVisible;

            [controller transformProcessToForeground];
            [NSApp unhide:nil];
            wasVisible = [self isVisible];

            if (activate)
                [NSApp activateIgnoringOtherApps:YES];

            NSDisableScreenUpdates();

            if ([self becameEligibleParentOrChild])
                needAdjustWindowLevels = TRUE;

            if (prev || next)
            {
                WineWindow* other = [prev isVisible] ? prev : next;
                NSWindowOrderingMode orderingMode = [prev isVisible] ? NSWindowBelow : NSWindowAbove;

                if (![self isOrdered:orderingMode relativeTo:other])
                {
                    WineWindow* parent = (WineWindow*)[self parentWindow];
                    WineWindow* otherParent = (WineWindow*)[other parentWindow];

                    // This window level may not be right for this window based
                    // on floating-ness, fullscreen-ness, etc.  But we set it
                    // temporarily to allow us to order the windows properly.
                    // Then the levels get fixed by -adjustWindowLevels.
                    if ([self level] != [other level])
                        [self setLevel:[other level]];
                    [self orderWindow:orderingMode relativeTo:[other windowNumber]];

                    // The above call to -[NSWindow orderWindow:relativeTo:] won't
                    // reorder windows which are both children of the same parent
                    // relative to each other, so do that separately.
                    if (parent && parent == otherParent)
                        [parent order:orderingMode childWindow:self relativeTo:other];

                    needAdjustWindowLevels = TRUE;
                }
            }
            else
            {
                // Again, temporarily set level to make sure we can order to
                // the right place.
                next = [controller frontWineWindow];
                if (next && [self level] < [next level])
                    [self setLevel:[next level]];
                [self orderFront:nil];
                needAdjustWindowLevels = TRUE;
            }

            if ([self becameEligibleParentOrChild])
                needAdjustWindowLevels = TRUE;

            if (needAdjustWindowLevels)
            {
                if (!wasVisible && fullscreen && [self isOnActiveSpace])
                    [controller updateFullscreenWindows];
                [controller adjustWindowLevels];
            }

            if (pendingMinimize)
            {
                [super miniaturize:nil];
                pendingMinimize = FALSE;
            }

            NSEnableScreenUpdates();

            /* Cocoa may adjust the frame when the window is ordered onto the screen.
               Generate a frame-changed event just in case.  The back end will ignore
               it if nothing actually changed. */
            [self windowDidResize:nil];

            if (![self isExcludedFromWindowsMenu])
                [NSApp addWindowsItem:self title:[self title] filename:NO];
        }
    }

    - (void) doOrderOut
    {
        WineApplicationController* controller = [WineApplicationController sharedController];
        BOOL wasVisible = [self isVisible];
        BOOL wasOnActiveSpace = [self isOnActiveSpace];

        if ([self isMiniaturized])
            pendingMinimize = TRUE;

        [self becameIneligibleParentOrChild];
        if ([self isMiniaturized])
        {
            fakingClose = TRUE;
            [self close];
            fakingClose = FALSE;
        }
        else
            [self orderOut:nil];
        savedVisibleState = FALSE;
        if (wasVisible && wasOnActiveSpace && fullscreen)
            [controller updateFullscreenWindows];
        [controller adjustWindowLevels];
        [NSApp removeWindowsItem:self];

        [queue discardEventsMatchingMask:event_mask_for_type(WINDOW_BROUGHT_FORWARD) |
                                         event_mask_for_type(WINDOW_GOT_FOCUS) |
                                         event_mask_for_type(WINDOW_LOST_FOCUS) |
                                         event_mask_for_type(WINDOW_MAXIMIZE_REQUESTED) |
                                         event_mask_for_type(WINDOW_MINIMIZE_REQUESTED) |
                                         event_mask_for_type(WINDOW_RESTORE_REQUESTED)
                               forWindow:self];
    }

    - (void) updateFullscreen
    {
        NSRect contentRect = [self contentRectForFrameRect:[self frame]];
        BOOL nowFullscreen = !([self styleMask] & NSFullScreenWindowMask) && screen_covered_by_rect(contentRect, [NSScreen screens]);

        if (nowFullscreen != fullscreen)
        {
            WineApplicationController* controller = [WineApplicationController sharedController];

            fullscreen = nowFullscreen;
            if ([self isVisible] && [self isOnActiveSpace])
                [controller updateFullscreenWindows];

            [controller adjustWindowLevels];
        }
    }

    - (void) setFrameFromWine:(NSRect)contentRect
    {
        /* Origin is (left, top) in a top-down space.  Need to convert it to
           (left, bottom) in a bottom-up space. */
        [[WineApplicationController sharedController] flipRect:&contentRect];

        /* The back end is establishing a new window size and position.  It's
           not interested in any stale events regarding those that may be sitting
           in the queue. */
        [queue discardEventsMatchingMask:event_mask_for_type(WINDOW_FRAME_CHANGED)
                               forWindow:self];

        if (!NSIsEmptyRect(contentRect))
        {
            NSRect frame, oldFrame;

            oldFrame = [self frame];
            frame = [self frameRectForContentRect:contentRect];
            if (!NSEqualRects(frame, oldFrame))
            {
                BOOL equalSizes = NSEqualSizes(frame.size, oldFrame.size);
                BOOL needEnableScreenUpdates = FALSE;

                if ([self preventResizing])
                {
                    // Allow the following calls to -setFrame:display: to work even
                    // if they would violate the content size constraints. This
                    // shouldn't be necessary since the content size constraints are
                    // documented to not constrain that method, but it seems to be.
                    [self setContentMinSize:NSZeroSize];
                    [self setContentMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];
                }

                if (equalSizes && [[self childWindows] count])
                {
                    // If we change the window frame such that the origin moves
                    // but the size doesn't change, then Cocoa moves child
                    // windows with the parent.  We don't want that so we fake
                    // a change of the size and then change it back.
                    NSRect bogusFrame = frame;
                    bogusFrame.size.width++;

                    NSDisableScreenUpdates();
                    needEnableScreenUpdates = TRUE;

                    ignore_windowResize = TRUE;
                    [self setFrame:bogusFrame display:NO];
                    ignore_windowResize = FALSE;
                }

                [self setFrame:frame display:YES];
                if ([self preventResizing])
                {
                    [self setContentMinSize:contentRect.size];
                    [self setContentMaxSize:contentRect.size];
                }

                if (needEnableScreenUpdates)
                    NSEnableScreenUpdates();

                if (!equalSizes)
                    [self updateColorSpace];

                if (!enteringFullScreen &&
                    [[NSProcessInfo processInfo] systemUptime] - enteredFullScreenTime > 1.0)
                    nonFullscreenFrame = frame;

                [self updateFullscreen];

                if ([self isOrderedIn])
                {
                    /* In case Cocoa adjusted the frame we tried to set, generate a frame-changed
                       event.  The back end will ignore it if nothing actually changed. */
                    [self windowDidResize:nil];
                }
            }
        }
    }

    - (void) setMacDrvParentWindow:(WineWindow*)parent
    {
        WineWindow* oldParent = (WineWindow*)[self parentWindow];
        if ((oldParent && oldParent != parent) || (!oldParent && latentParentWindow != parent))
        {
            [oldParent removeChildWineWindow:self];
            [latentParentWindow removeChildWineWindow:self];
            if ([parent addChildWineWindow:self])
                [[WineApplicationController sharedController] adjustWindowLevels];
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

    - (void) setLiveResizeDisplayTimer:(NSTimer*)newTimer
    {
        if (newTimer != liveResizeDisplayTimer)
        {
            [liveResizeDisplayTimer invalidate];
            [liveResizeDisplayTimer release];
            liveResizeDisplayTimer = [newTimer retain];
        }
    }

    - (void) makeFocused:(BOOL)activate
    {
        if (activate)
        {
            [[WineApplicationController sharedController] transformProcessToForeground];
            [NSApp activateIgnoringOtherApps:YES];
        }

        causing_becomeKeyWindow = self;
        [self makeKeyWindow];
        causing_becomeKeyWindow = nil;

        [queue discardEventsMatchingMask:event_mask_for_type(WINDOW_GOT_FOCUS) |
                                         event_mask_for_type(WINDOW_LOST_FOCUS)
                               forWindow:self];
    }

    - (void) postKey:(uint16_t)keyCode
             pressed:(BOOL)pressed
           modifiers:(NSUInteger)modifiers
               event:(NSEvent*)theEvent
    {
        macdrv_event* event;
        CGEventRef cgevent;
        WineApplicationController* controller = [WineApplicationController sharedController];

        event = macdrv_create_event(pressed ? KEY_PRESS : KEY_RELEASE, self);
        event->key.keycode   = keyCode;
        event->key.modifiers = modifiers;
        event->key.time_ms   = [controller ticksForEventTime:[theEvent timestamp]];

        if ((cgevent = [theEvent CGEvent]))
        {
            CGEventSourceKeyboardType keyboardType = CGEventGetIntegerValueField(cgevent,
                                                        kCGKeyboardEventKeyboardType);
            if (keyboardType != controller.keyboardType)
            {
                controller.keyboardType = keyboardType;
                [controller keyboardSelectionDidChange];
            }
        }

        [queue postEvent:event];

        macdrv_release_event(event);

        [controller noteKey:keyCode pressed:pressed];
    }

    - (void) postKeyEvent:(NSEvent *)theEvent
    {
        [self flagsChanged:theEvent];
        [self postKey:[theEvent keyCode]
              pressed:[theEvent type] == NSKeyDown
            modifiers:adjusted_modifiers_for_option_behavior([theEvent modifierFlags])
                event:theEvent];
    }

    - (void) setWineMinSize:(NSSize)minSize maxSize:(NSSize)maxSize
    {
        savedContentMinSize = minSize;
        savedContentMaxSize = maxSize;
        if (![self preventResizing])
        {
            [self setContentMinSize:minSize];
            [self setContentMaxSize:maxSize];
        }
    }

    - (WineWindow*) ancestorWineWindow
    {
        WineWindow* ancestor = self;
        for (;;)
        {
            WineWindow* parent = (WineWindow*)[ancestor parentWindow];
            if ([parent isKindOfClass:[WineWindow class]])
                ancestor = parent;
            else
                break;
        }
        return ancestor;
    }

    - (void) postBroughtForwardEvent
    {
        macdrv_event* event = macdrv_create_event(WINDOW_BROUGHT_FORWARD, self);
        [queue postEvent:event];
        macdrv_release_event(event);
    }

    - (void) updateForCursorClipping
    {
        [self adjustFeaturesForState];
    }


    /*
     * ---------- NSWindow method overrides ----------
     */
    - (BOOL) canBecomeKeyWindow
    {
        if (causing_becomeKeyWindow == self) return YES;
        if (self.disabled || self.noActivate) return NO;
        return [self isKeyWindow];
    }

    - (BOOL) canBecomeMainWindow
    {
        return [self canBecomeKeyWindow];
    }

    - (NSRect) constrainFrameRect:(NSRect)frameRect toScreen:(NSScreen *)screen
    {
        // If a window is sized to completely cover a screen, then it's in
        // full-screen mode.  In that case, we don't allow NSWindow to constrain
        // it.
        NSArray* screens = [NSScreen screens];
        NSRect contentRect = [self contentRectForFrameRect:frameRect];
        if (!screen_covered_by_rect(contentRect, screens) &&
            frame_intersects_screens(frameRect, screens))
            frameRect = [super constrainFrameRect:frameRect toScreen:screen];
        return frameRect;
    }

    - (BOOL) isExcludedFromWindowsMenu
    {
        return !([self collectionBehavior] & NSWindowCollectionBehaviorParticipatesInCycle);
    }

    - (BOOL) validateMenuItem:(NSMenuItem *)menuItem
    {
        BOOL ret = [super validateMenuItem:menuItem];

        if ([menuItem action] == @selector(makeKeyAndOrderFront:))
            ret = [self isKeyWindow] || (!self.disabled && !self.noActivate);
        if ([menuItem action] == @selector(toggleFullScreen:) && self.disabled)
            ret = NO;

        return ret;
    }

    /* We don't call this.  It's the action method of the items in the Window menu. */
    - (void) makeKeyAndOrderFront:(id)sender
    {
        if ([self isMiniaturized])
            [self deminiaturize:nil];
        [self orderBelow:nil orAbove:nil activate:NO];
        [[self ancestorWineWindow] postBroughtForwardEvent];

        if (![self isKeyWindow] && !self.disabled && !self.noActivate)
            [[WineApplicationController sharedController] windowGotFocus:self];
    }

    - (void) sendEvent:(NSEvent*)event
    {
        /* NSWindow consumes certain key-down events as part of Cocoa's keyboard
           interface control.  For example, Control-Tab switches focus among
           views.  We want to bypass that feature, so directly route key-down
           events to -keyDown:. */
        if ([event type] == NSKeyDown)
            [[self firstResponder] keyDown:event];
        else
            [super sendEvent:event];
    }

    - (void) miniaturize:(id)sender
    {
        macdrv_event* event = macdrv_create_event(WINDOW_MINIMIZE_REQUESTED, self);
        [queue postEvent:event];
        macdrv_release_event(event);
    }

    - (void) toggleFullScreen:(id)sender
    {
        if (!self.disabled)
            [super toggleFullScreen:sender];
    }

    // We normally use the generic/calibrated RGB color space for the window,
    // rather than the device color space, to avoid expensive color conversion
    // which slows down drawing.  However, for windows displaying OpenGL, having
    // a different color space than the screen greatly reduces frame rates, often
    // limiting it to the display refresh rate.
    //
    // To avoid this, we switch back to the screen color space whenever the
    // window is covered by a view with an attached OpenGL context.
    - (void) updateColorSpace
    {
        NSRect contentRect = [[self contentView] frame];
        BOOL coveredByGLView = FALSE;
        for (WineContentView* view in [[self contentView] subviews])
        {
            if ([view hasGLContext])
            {
                NSRect frame = [view convertRect:[view bounds] toView:nil];
                if (NSContainsRect(frame, contentRect))
                {
                    coveredByGLView = TRUE;
                    break;
                }
            }
        }

        if (coveredByGLView)
            [self setColorSpace:nil];
        else
            [self setColorSpace:[NSColorSpace genericRGBColorSpace]];
    }


    /*
     * ---------- NSResponder method overrides ----------
     */
    - (void) keyDown:(NSEvent *)theEvent { [self postKeyEvent:theEvent]; }

    - (void) flagsChanged:(NSEvent *)theEvent
    {
        static const struct {
            NSUInteger  mask;
            uint16_t    keycode;
        } modifiers[] = {
            { NX_ALPHASHIFTMASK,        kVK_CapsLock },
            { NX_DEVICELSHIFTKEYMASK,   kVK_Shift },
            { NX_DEVICERSHIFTKEYMASK,   kVK_RightShift },
            { NX_DEVICELCTLKEYMASK,     kVK_Control },
            { NX_DEVICERCTLKEYMASK,     kVK_RightControl },
            { NX_DEVICELALTKEYMASK,     kVK_Option },
            { NX_DEVICERALTKEYMASK,     kVK_RightOption },
            { NX_DEVICELCMDKEYMASK,     kVK_Command },
            { NX_DEVICERCMDKEYMASK,     kVK_RightCommand },
        };

        NSUInteger modifierFlags = adjusted_modifiers_for_option_behavior([theEvent modifierFlags]);
        NSUInteger changed;
        int i, last_changed;

        fix_device_modifiers_by_generic(&modifierFlags);
        changed = modifierFlags ^ lastModifierFlags;

        last_changed = -1;
        for (i = 0; i < sizeof(modifiers)/sizeof(modifiers[0]); i++)
            if (changed & modifiers[i].mask)
                last_changed = i;

        for (i = 0; i <= last_changed; i++)
        {
            if (changed & modifiers[i].mask)
            {
                BOOL pressed = (modifierFlags & modifiers[i].mask) != 0;

                if (i == last_changed)
                    lastModifierFlags = modifierFlags;
                else
                {
                    lastModifierFlags ^= modifiers[i].mask;
                    fix_generic_modifiers_by_device(&lastModifierFlags);
                }

                // Caps lock generates one event for each press-release action.
                // We need to simulate a pair of events for each actual event.
                if (modifiers[i].mask == NX_ALPHASHIFTMASK)
                {
                    [self postKey:modifiers[i].keycode
                          pressed:TRUE
                        modifiers:lastModifierFlags
                            event:(NSEvent*)theEvent];
                    pressed = FALSE;
                }

                [self postKey:modifiers[i].keycode
                      pressed:pressed
                    modifiers:lastModifierFlags
                        event:(NSEvent*)theEvent];
            }
        }
    }

    - (void) applicationWillHide
    {
        savedVisibleState = [self isVisible];
    }

    - (void) applicationDidUnhide
    {
        if ([self isVisible])
            [self becameEligibleParentOrChild];
    }


    /*
     * ---------- NSWindowDelegate methods ----------
     */
    - (NSSize) window:(NSWindow*)window willUseFullScreenContentSize:(NSSize)proposedSize
    {
        macdrv_query* query;
        NSSize size;

        query = macdrv_create_query();
        query->type = QUERY_MIN_MAX_INFO;
        query->window = (macdrv_window)[self retain];
        [self.queue query:query timeout:0.5];
        macdrv_release_query(query);

        size = [self contentMaxSize];
        if (proposedSize.width < size.width)
            size.width = proposedSize.width;
        if (proposedSize.height < size.height)
            size.height = proposedSize.height;
        return size;
    }

    - (void)windowDidBecomeKey:(NSNotification *)notification
    {
        WineApplicationController* controller = [WineApplicationController sharedController];
        NSEvent* event = [controller lastFlagsChanged];
        if (event)
            [self flagsChanged:event];

        if (causing_becomeKeyWindow == self) return;

        [controller windowGotFocus:self];
    }

    - (void)windowDidDeminiaturize:(NSNotification *)notification
    {
        WineApplicationController* controller = [WineApplicationController sharedController];

        if (!ignore_windowDeminiaturize)
            [self postDidUnminimizeEvent];
        ignore_windowDeminiaturize = FALSE;

        [self becameEligibleParentOrChild];

        if (fullscreen && [self isOnActiveSpace])
            [controller updateFullscreenWindows];
        [controller adjustWindowLevels];

        if (![self parentWindow])
            [self postBroughtForwardEvent];

        if (!self.disabled && !self.noActivate)
        {
            causing_becomeKeyWindow = self;
            [self makeKeyWindow];
            causing_becomeKeyWindow = nil;
            [controller windowGotFocus:self];
        }

        [self windowDidResize:notification];
    }

    - (void) windowDidEndLiveResize:(NSNotification *)notification
    {
        macdrv_event* event = macdrv_create_event(WINDOW_RESIZE_ENDED, self);
        [queue postEvent:event];
        macdrv_release_event(event);

        self.liveResizeDisplayTimer = nil;
    }

    - (void) windowDidEnterFullScreen:(NSNotification*)notification
    {
        enteringFullScreen = FALSE;
        enteredFullScreenTime = [[NSProcessInfo processInfo] systemUptime];
    }

    - (void) windowDidExitFullScreen:(NSNotification*)notification
    {
        exitingFullScreen = FALSE;
        [self setFrame:nonFullscreenFrame display:YES animate:NO];
        [self windowDidResize:nil];
    }

    - (void) windowDidFailToEnterFullScreen:(NSWindow*)window
    {
        enteringFullScreen = FALSE;
        enteredFullScreenTime = 0;
    }

    - (void) windowDidFailToExitFullScreen:(NSWindow*)window
    {
        exitingFullScreen = FALSE;
        [self windowDidResize:nil];
    }

    - (void)windowDidMiniaturize:(NSNotification *)notification
    {
        if (fullscreen && [self isOnActiveSpace])
            [[WineApplicationController sharedController] updateFullscreenWindows];
    }

    - (void)windowDidMove:(NSNotification *)notification
    {
        [self windowDidResize:notification];
    }

    - (void)windowDidResignKey:(NSNotification *)notification
    {
        macdrv_event* event;

        if (causing_becomeKeyWindow) return;

        event = macdrv_create_event(WINDOW_LOST_FOCUS, self);
        [queue postEvent:event];
        macdrv_release_event(event);
    }

    - (void)windowDidResize:(NSNotification *)notification
    {
        macdrv_event* event;
        NSRect frame = [self frame];

        if ([self inLiveResize])
        {
            if (NSMinX(frame) != NSMinX(frameAtResizeStart))
                resizingFromLeft = TRUE;
            if (NSMaxY(frame) != NSMaxY(frameAtResizeStart))
                resizingFromTop = TRUE;
        }

        frame = [self contentRectForFrameRect:frame];

        if (ignore_windowResize || exitingFullScreen) return;

        if ([self preventResizing])
        {
            [self setContentMinSize:frame.size];
            [self setContentMaxSize:frame.size];
        }

        [[WineApplicationController sharedController] flipRect:&frame];

        /* Coalesce events by discarding any previous ones still in the queue. */
        [queue discardEventsMatchingMask:event_mask_for_type(WINDOW_FRAME_CHANGED)
                               forWindow:self];

        event = macdrv_create_event(WINDOW_FRAME_CHANGED, self);
        event->window_frame_changed.frame = NSRectToCGRect(frame);
        event->window_frame_changed.fullscreen = ([self styleMask] & NSFullScreenWindowMask) != 0;
        event->window_frame_changed.in_resize = [self inLiveResize];
        [queue postEvent:event];
        macdrv_release_event(event);

        [[[self contentView] inputContext] invalidateCharacterCoordinates];
        [self updateFullscreen];
    }

    - (BOOL)windowShouldClose:(id)sender
    {
        macdrv_event* event = macdrv_create_event(WINDOW_CLOSE_REQUESTED, self);
        [queue postEvent:event];
        macdrv_release_event(event);
        return NO;
    }

    - (BOOL) windowShouldZoom:(NSWindow*)window toFrame:(NSRect)newFrame
    {
        if (maximized)
        {
            macdrv_event* event = macdrv_create_event(WINDOW_RESTORE_REQUESTED, self);
            [queue postEvent:event];
            macdrv_release_event(event);
            return NO;
        }
        else if (!resizable)
        {
            macdrv_event* event = macdrv_create_event(WINDOW_MAXIMIZE_REQUESTED, self);
            [queue postEvent:event];
            macdrv_release_event(event);
            return NO;
        }

        return YES;
    }

    - (void) windowWillClose:(NSNotification*)notification
    {
        WineWindow* child;

        if (fakingClose) return;
        if (latentParentWindow)
        {
            [latentParentWindow->latentChildWindows removeObjectIdenticalTo:self];
            self.latentParentWindow = nil;
        }

        for (child in latentChildWindows)
        {
            if (child.latentParentWindow == self)
                child.latentParentWindow = nil;
        }
        [latentChildWindows removeAllObjects];
    }

    - (void) windowWillEnterFullScreen:(NSNotification*)notification
    {
        enteringFullScreen = TRUE;
        nonFullscreenFrame = [self frame];
    }

    - (void) windowWillExitFullScreen:(NSNotification*)notification
    {
        exitingFullScreen = TRUE;
    }

    - (void)windowWillMiniaturize:(NSNotification *)notification
    {
        [self becameIneligibleParentOrChild];
    }

    - (NSSize) windowWillResize:(NSWindow*)sender toSize:(NSSize)frameSize
    {
        if ([self inLiveResize])
        {
            NSRect rect;
            macdrv_query* query;

            rect = [self frame];
            if (resizingFromLeft)
                rect.origin.x = NSMaxX(rect) - frameSize.width;
            if (!resizingFromTop)
                rect.origin.y = NSMaxY(rect) - frameSize.height;
            rect.size = frameSize;
            rect = [self contentRectForFrameRect:rect];
            [[WineApplicationController sharedController] flipRect:&rect];

            query = macdrv_create_query();
            query->type = QUERY_RESIZE_SIZE;
            query->window = (macdrv_window)[self retain];
            query->resize_size.rect = NSRectToCGRect(rect);
            query->resize_size.from_left = resizingFromLeft;
            query->resize_size.from_top = resizingFromTop;

            if ([self.queue query:query timeout:0.1])
            {
                rect = NSRectFromCGRect(query->resize_size.rect);
                rect = [self frameRectForContentRect:rect];
                frameSize = rect.size;
            }

            macdrv_release_query(query);
        }

        return frameSize;
    }

    - (void) windowWillStartLiveResize:(NSNotification *)notification
    {
        macdrv_query* query = macdrv_create_query();
        query->type = QUERY_RESIZE_START;
        query->window = (macdrv_window)[self retain];

        [self.queue query:query timeout:0.3];
        macdrv_release_query(query);

        frameAtResizeStart = [self frame];
        resizingFromLeft = resizingFromTop = FALSE;

        // There's a strange restriction in window redrawing during Cocoa-
        // managed window resizing.  Only calls to -[NSView setNeedsDisplay...]
        // that happen synchronously when Cocoa tells us that our window size
        // has changed or asynchronously in a short interval thereafter provoke
        // the window to redraw.  Calls to those methods that happen asynchronously
        // a half second or more after the last change of the window size aren't
        // heeded until the next resize-related user event (e.g. mouse movement).
        //
        // Wine often has a significant delay between when it's been told that
        // the window has changed size and when it can flush completed drawing.
        // So, our windows would get stuck with incomplete drawing for as long
        // as the user holds the mouse button down and doesn't move it.
        //
        // We address this by "manually" asking our windows to check if they need
        // redrawing every so often (during live resize only).
        self.liveResizeDisplayTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/30.0
                                                                       target:self
                                                                     selector:@selector(displayIfNeeded)
                                                                     userInfo:nil
                                                                      repeats:YES];
        [[NSRunLoop currentRunLoop] addTimer:liveResizeDisplayTimer
                                     forMode:NSRunLoopCommonModes];
    }

    - (NSRect) windowWillUseStandardFrame:(NSWindow*)window defaultFrame:(NSRect)proposedFrame
    {
        macdrv_query* query;
        NSRect currentContentRect, proposedContentRect, newContentRect, screenRect;
        NSSize maxSize;

        query = macdrv_create_query();
        query->type = QUERY_MIN_MAX_INFO;
        query->window = (macdrv_window)[self retain];
        [self.queue query:query timeout:0.5];
        macdrv_release_query(query);

        currentContentRect = [self contentRectForFrameRect:[self frame]];
        proposedContentRect = [self contentRectForFrameRect:proposedFrame];

        maxSize = [self contentMaxSize];
        newContentRect.size.width = MIN(NSWidth(proposedContentRect), maxSize.width);
        newContentRect.size.height = MIN(NSHeight(proposedContentRect), maxSize.height);

        // Try to keep the top-left corner where it is.
        newContentRect.origin.x = NSMinX(currentContentRect);
        newContentRect.origin.y = NSMaxY(currentContentRect) - NSHeight(newContentRect);

        // If that pushes the bottom or right off the screen, pull it up and to the left.
        screenRect = [self contentRectForFrameRect:[[self screen] visibleFrame]];
        if (NSMaxX(newContentRect) > NSMaxX(screenRect))
            newContentRect.origin.x = NSMaxX(screenRect) - NSWidth(newContentRect);
        if (NSMinY(newContentRect) < NSMinY(screenRect))
            newContentRect.origin.y = NSMinY(screenRect);

        // If that pushes the top or left off the screen, push it down and the right
        // again.  Do this last because the top-left corner is more important than the
        // bottom-right.
        if (NSMinX(newContentRect) < NSMinX(screenRect))
            newContentRect.origin.x = NSMinX(screenRect);
        if (NSMaxY(newContentRect) > NSMaxY(screenRect))
            newContentRect.origin.y = NSMaxY(screenRect) - NSHeight(newContentRect);

        return [self frameRectForContentRect:newContentRect];
    }


    /*
     * ---------- NSPasteboardOwner methods ----------
     */
    - (void) pasteboard:(NSPasteboard *)sender provideDataForType:(NSString *)type
    {
        macdrv_query* query = macdrv_create_query();
        query->type = QUERY_PASTEBOARD_DATA;
        query->window = (macdrv_window)[self retain];
        query->pasteboard_data.type = (CFStringRef)[type copy];

        [self.queue query:query timeout:3];
        macdrv_release_query(query);
    }


    /*
     * ---------- NSDraggingDestination methods ----------
     */
    - (NSDragOperation) draggingEntered:(id <NSDraggingInfo>)sender
    {
        return [self draggingUpdated:sender];
    }

    - (void) draggingExited:(id <NSDraggingInfo>)sender
    {
        // This isn't really a query.  We don't need any response.  However, it
        // has to be processed in a similar manner as the other drag-and-drop
        // queries in order to maintain the proper order of operations.
        macdrv_query* query = macdrv_create_query();
        query->type = QUERY_DRAG_EXITED;
        query->window = (macdrv_window)[self retain];

        [self.queue query:query timeout:0.1];
        macdrv_release_query(query);
    }

    - (NSDragOperation) draggingUpdated:(id <NSDraggingInfo>)sender
    {
        NSDragOperation ret;
        NSPoint pt = [[self contentView] convertPoint:[sender draggingLocation] fromView:nil];
        NSPasteboard* pb = [sender draggingPasteboard];

        macdrv_query* query = macdrv_create_query();
        query->type = QUERY_DRAG_OPERATION;
        query->window = (macdrv_window)[self retain];
        query->drag_operation.x = pt.x;
        query->drag_operation.y = pt.y;
        query->drag_operation.offered_ops = [sender draggingSourceOperationMask];
        query->drag_operation.accepted_op = NSDragOperationNone;
        query->drag_operation.pasteboard = (CFTypeRef)[pb retain];

        [self.queue query:query timeout:3];
        ret = query->status ? query->drag_operation.accepted_op : NSDragOperationNone;
        macdrv_release_query(query);

        return ret;
    }

    - (BOOL) performDragOperation:(id <NSDraggingInfo>)sender
    {
        BOOL ret;
        NSPoint pt = [[self contentView] convertPoint:[sender draggingLocation] fromView:nil];
        NSPasteboard* pb = [sender draggingPasteboard];

        macdrv_query* query = macdrv_create_query();
        query->type = QUERY_DRAG_DROP;
        query->window = (macdrv_window)[self retain];
        query->drag_drop.x = pt.x;
        query->drag_drop.y = pt.y;
        query->drag_drop.op = [sender draggingSourceOperationMask];
        query->drag_drop.pasteboard = (CFTypeRef)[pb retain];

        [self.queue query:query timeout:3 * 60 processEvents:YES];
        ret = query->status;
        macdrv_release_query(query);

        return ret;
    }

    - (BOOL) wantsPeriodicDraggingUpdates
    {
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

    OnMainThread(^{
        [window doOrderOut];
        [window close];
    });
    [window.queue discardEventsMatchingMask:-1 forWindow:window];
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
        if ([window isOrderedIn] && ![window isExcludedFromWindowsMenu])
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
 */
void macdrv_order_cocoa_window(macdrv_window w, macdrv_window p,
        macdrv_window n, int activate)
{
    WineWindow* window = (WineWindow*)w;
    WineWindow* prev = (WineWindow*)p;
    WineWindow* next = (WineWindow*)n;

    OnMainThreadAsync(^{
        [window orderBelow:prev
                   orAbove:next
                  activate:activate];
    });
    [window.queue discardEventsMatchingMask:event_mask_for_type(WINDOW_BROUGHT_FORWARD)
                                  forWindow:window];
    [next.queue discardEventsMatchingMask:event_mask_for_type(WINDOW_BROUGHT_FORWARD)
                                forWindow:next];
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
 * Move a Cocoa window.
 */
void macdrv_set_cocoa_window_frame(macdrv_window w, const CGRect* new_frame)
{
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        [window setFrameFromWine:NSRectFromCGRect(*new_frame)];
    });
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
        [[WineApplicationController sharedController] flipRect:&frame];
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
void macdrv_give_cocoa_window_focus(macdrv_window w, int activate)
{
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        [window makeFocused:activate];
    });
}

/***********************************************************************
 *              macdrv_set_window_min_max_sizes
 *
 * Sets the window's minimum and maximum content sizes.
 */
void macdrv_set_window_min_max_sizes(macdrv_window w, CGSize min_size, CGSize max_size)
{
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        [window setWineMinSize:NSSizeFromCGSize(min_size) maxSize:NSSizeFromCGSize(max_size)];
    });
}

/***********************************************************************
 *              macdrv_create_view
 *
 * Creates and returns a view in the specified rect of the window.  The
 * caller is responsible for calling macdrv_dispose_view() on the view
 * when it is done with it.
 */
macdrv_view macdrv_create_view(macdrv_window w, CGRect rect)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineWindow* window = (WineWindow*)w;
    __block WineContentView* view;

    if (CGRectIsNull(rect)) rect = CGRectZero;

    OnMainThread(^{
        NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];

        view = [[WineContentView alloc] initWithFrame:NSRectFromCGRect(rect)];
        [view setAutoresizesSubviews:NO];
        [nc addObserver:view
               selector:@selector(updateGLContexts)
                   name:NSViewGlobalFrameDidChangeNotification
                 object:view];
        [nc addObserver:view
               selector:@selector(updateGLContexts)
                   name:NSApplicationDidChangeScreenParametersNotification
                 object:NSApp];
        [[window contentView] addSubview:view];
        [window updateColorSpace];
    });

    [pool release];
    return (macdrv_view)view;
}

/***********************************************************************
 *              macdrv_dispose_view
 *
 * Destroys a view previously returned by macdrv_create_view.
 */
void macdrv_dispose_view(macdrv_view v)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineContentView* view = (WineContentView*)v;

    OnMainThread(^{
        NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
        WineWindow* window = (WineWindow*)[view window];

        [nc removeObserver:view
                      name:NSViewGlobalFrameDidChangeNotification
                    object:view];
        [nc removeObserver:view
                      name:NSApplicationDidChangeScreenParametersNotification
                    object:NSApp];
        [view removeFromSuperview];
        [view release];
        [window updateColorSpace];
    });

    [pool release];
}

/***********************************************************************
 *              macdrv_set_view_window_and_frame
 *
 * Move a view to a new window and/or position within its window.  If w
 * is NULL, leave the view in its current window and just change its
 * frame.
 */
void macdrv_set_view_window_and_frame(macdrv_view v, macdrv_window w, CGRect rect)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineContentView* view = (WineContentView*)v;
    WineWindow* window = (WineWindow*)w;

    if (CGRectIsNull(rect)) rect = CGRectZero;

    OnMainThread(^{
        BOOL changedWindow = (window && window != [view window]);
        NSRect newFrame = NSRectFromCGRect(rect);
        NSRect oldFrame = [view frame];
        BOOL needUpdateWindowColorSpace = FALSE;

        if (changedWindow)
        {
            WineWindow* oldWindow = (WineWindow*)[view window];
            [view removeFromSuperview];
            [oldWindow updateColorSpace];
            [[window contentView] addSubview:view];
            needUpdateWindowColorSpace = TRUE;
        }

        if (!NSEqualRects(oldFrame, newFrame))
        {
            if (!changedWindow)
                [[view superview] setNeedsDisplayInRect:oldFrame];
            if (NSEqualPoints(oldFrame.origin, newFrame.origin))
                [view setFrameSize:newFrame.size];
            else if (NSEqualSizes(oldFrame.size, newFrame.size))
                [view setFrameOrigin:newFrame.origin];
            else
                [view setFrame:newFrame];
            [view setNeedsDisplay:YES];
            needUpdateWindowColorSpace = TRUE;
        }

        if (needUpdateWindowColorSpace)
            [(WineWindow*)[view window] updateColorSpace];
    });

    [pool release];
}

/***********************************************************************
 *              macdrv_add_view_opengl_context
 *
 * Add an OpenGL context to the list being tracked for each view.
 */
void macdrv_add_view_opengl_context(macdrv_view v, macdrv_opengl_context c)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineContentView* view = (WineContentView*)v;
    WineOpenGLContext *context = (WineOpenGLContext*)c;

    OnMainThread(^{
        [view addGLContext:context];
    });

    [pool release];
}

/***********************************************************************
 *              macdrv_remove_view_opengl_context
 *
 * Add an OpenGL context to the list being tracked for each view.
 */
void macdrv_remove_view_opengl_context(macdrv_view v, macdrv_opengl_context c)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineContentView* view = (WineContentView*)v;
    WineOpenGLContext *context = (WineOpenGLContext*)c;

    OnMainThreadAsync(^{
        [view removeGLContext:context];
    });

    [pool release];
}

/***********************************************************************
 *              macdrv_window_background_color
 *
 * Returns the standard Mac window background color as a 32-bit value of
 * the form 0x00rrggbb.
 */
uint32_t macdrv_window_background_color(void)
{
    static uint32_t result;
    static dispatch_once_t once;

    // Annoyingly, [NSColor windowBackgroundColor] refuses to convert to other
    // color spaces (RGB or grayscale).  So, the only way to get RGB values out
    // of it is to draw with it.
    dispatch_once(&once, ^{
        OnMainThread(^{
            unsigned char rgbx[4];
            unsigned char *planes = rgbx;
            NSBitmapImageRep *bitmap = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:&planes
                                                                               pixelsWide:1
                                                                               pixelsHigh:1
                                                                            bitsPerSample:8
                                                                          samplesPerPixel:3
                                                                                 hasAlpha:NO
                                                                                 isPlanar:NO
                                                                           colorSpaceName:NSCalibratedRGBColorSpace
                                                                             bitmapFormat:0
                                                                              bytesPerRow:4
                                                                             bitsPerPixel:32];
            [NSGraphicsContext saveGraphicsState];
            [NSGraphicsContext setCurrentContext:[NSGraphicsContext graphicsContextWithBitmapImageRep:bitmap]];
            [[NSColor windowBackgroundColor] set];
            NSRectFill(NSMakeRect(0, 0, 1, 1));
            [NSGraphicsContext restoreGraphicsState];
            [bitmap release];
            result = rgbx[0] << 16 | rgbx[1] << 8 | rgbx[2];
        });
    });

    return result;
}

/***********************************************************************
 *              macdrv_send_text_input_event
 */
int macdrv_send_text_input_event(int pressed, unsigned int flags, int repeat, int keyc, void* data)
{
    __block BOOL ret;

    OnMainThread(^{
        WineWindow* window = (WineWindow*)[NSApp keyWindow];
        if (![window isKindOfClass:[WineWindow class]])
        {
            window = (WineWindow*)[NSApp mainWindow];
            if (![window isKindOfClass:[WineWindow class]])
                window = [[WineApplicationController sharedController] frontWineWindow];
        }

        if (window)
        {
            NSUInteger localFlags = flags;
            CGEventRef c;
            NSEvent* event;

            window.imeData = data;
            fix_device_modifiers_by_generic(&localFlags);

            // An NSEvent created with +keyEventWithType:... is internally marked
            // as synthetic and doesn't get sent through input methods.  But one
            // created from a CGEvent doesn't have that problem.
            c = CGEventCreateKeyboardEvent(NULL, keyc, pressed);
            CGEventSetFlags(c, localFlags);
            CGEventSetIntegerValueField(c, kCGKeyboardEventAutorepeat, repeat);
            event = [NSEvent eventWithCGEvent:c];
            CFRelease(c);

            window.commandDone = FALSE;
            ret = [[[window contentView] inputContext] handleEvent:event] && !window.commandDone;
        }
        else
            ret = FALSE;
    });

    return ret;
}
