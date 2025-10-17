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

#include "config.h"

#define GL_SILENCE_DEPRECATION
#import <CoreVideo/CoreVideo.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#include <dlfcn.h>

#import "cocoa_window.h"

#include "macdrv_cocoa.h"
#import "cocoa_app.h"
#import "cocoa_event.h"
#import "cocoa_opengl.h"

#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"


@interface NSWindow (PrivatePreventsActivation)

/* Needed to ensure proper behavior after adding or removing
 * NSWindowStyleMaskNonactivatingPanel.
 * Available since at least macOS 10.6. */
- (void)_setPreventsActivation:(BOOL)flag;

@end


static NSUInteger style_mask_for_features(const struct macdrv_window_features* wf)
{
    NSUInteger style_mask;

    if (wf->title_bar)
    {
        style_mask = NSWindowStyleMaskTitled;
        if (wf->close_button) style_mask |= NSWindowStyleMaskClosable;
        if (wf->minimize_button) style_mask |= NSWindowStyleMaskMiniaturizable;
        if (wf->resizable || wf->maximize_button) style_mask |= NSWindowStyleMaskResizable;
        if (wf->utility) style_mask |= NSWindowStyleMaskUtilityWindow;
    }
    else style_mask = NSWindowStyleMaskBorderless;

    if (wf->prevents_app_activation) style_mask |= NSWindowStyleMaskNonactivatingPanel;

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

static inline NSUInteger adjusted_modifiers_for_settings(NSUInteger modifiers)
{
    fix_device_modifiers_by_generic(&modifiers);
    NSUInteger new_modifiers = modifiers & ~(NX_DEVICELALTKEYMASK | NX_DEVICERALTKEYMASK |
                                             NX_DEVICELCMDKEYMASK | NX_DEVICERCMDKEYMASK);

    // The MACDRV keyboard driver translates Command keys to Alt. If the
    // Option key (NX_DEVICE[LR]ALTKEYMASK) should behave like Alt in
    // Windows, rewrite it to Command (NX_DEVICE[LR]CMDKEYMASK).
    if (modifiers & NX_DEVICELALTKEYMASK)
        new_modifiers |= left_option_is_alt ? NX_DEVICELCMDKEYMASK : NX_DEVICELALTKEYMASK;
    if (modifiers & NX_DEVICERALTKEYMASK)
        new_modifiers |= right_option_is_alt ? NX_DEVICERCMDKEYMASK : NX_DEVICERALTKEYMASK;

    if (modifiers & NX_DEVICELCMDKEYMASK)
        new_modifiers |= left_command_is_ctrl ? NX_DEVICELCTLKEYMASK : NX_DEVICELCMDKEYMASK;
    if (modifiers & NX_DEVICERCMDKEYMASK)
        new_modifiers |= right_command_is_ctrl ? NX_DEVICERCTLKEYMASK : NX_DEVICERCMDKEYMASK;

    fix_generic_modifiers_by_device(&new_modifiers);
    return new_modifiers;
}

static inline BOOL stage_manager_enabled(void)
{
    /* There is no documented way to determine if Stage Manager is enabled,
     * but this seems like the best option.
     */
    if (floor(NSAppKitVersionNumber) >= 2299 /* NSAppKitVersionNumber13_0 */)
    {
        NSUserDefaults *defs = [[NSUserDefaults alloc] initWithSuiteName:@"com.apple.WindowManager.plist"];
        BOOL enabled = [defs boolForKey:@"GloballyEnabled"];
        [defs release];
        return enabled;
    }
    return FALSE;
}


@interface NSWindow (WineAccessPrivateMethods)
    - (id) _displayChanged;
@end


@interface WineDisplayLink : NSObject
{
    CGDirectDisplayID _displayID;
    CVDisplayLinkRef _link;
    NSMutableSet* _windows;

    NSTimeInterval _actualRefreshPeriod;
    NSTimeInterval _nominalRefreshPeriod;

    NSTimeInterval _lastDisplayTime;
}

    - (id) initWithDisplayID:(CGDirectDisplayID)displayID;

    - (void) addWindow:(WineWindow*)window;
    - (void) removeWindow:(WineWindow*)window;

    - (NSTimeInterval) refreshPeriod;

    - (void) start;

@end

@implementation WineDisplayLink

static CVReturn WineDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* inNow, const CVTimeStamp* inOutputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext);

    - (id) initWithDisplayID:(CGDirectDisplayID)displayID
    {
        self = [super init];
        if (self)
        {
            CVReturn status = CVDisplayLinkCreateWithCGDisplay(displayID, &_link);
            if (status == kCVReturnSuccess && !_link)
                status = kCVReturnError;
            if (status == kCVReturnSuccess)
                status = CVDisplayLinkSetOutputCallback(_link, WineDisplayLinkCallback, self);
            if (status != kCVReturnSuccess)
            {
                [self release];
                return nil;
            }

            _displayID = displayID;
            _windows = [[NSMutableSet alloc] init];
        }
        return self;
    }

    - (void) dealloc
    {
        if (_link)
        {
            CVDisplayLinkStop(_link);
            CVDisplayLinkRelease(_link);
        }
        [_windows release];
        [super dealloc];
    }

    - (void) addWindow:(WineWindow*)window
    {
        BOOL firstWindow;
        @synchronized(self) {
            firstWindow = !_windows.count;
            [_windows addObject:window];
        }
        if (firstWindow || !CVDisplayLinkIsRunning(_link))
            [self start];
    }

    - (void) removeWindow:(WineWindow*)window
    {
        BOOL lastWindow = FALSE;
        @synchronized(self) {
            BOOL hadWindows = _windows.count > 0;
            [_windows removeObject:window];
            if (hadWindows && !_windows.count)
                lastWindow = TRUE;
        }
        if (lastWindow && CVDisplayLinkIsRunning(_link))
            CVDisplayLinkStop(_link);
    }

    - (void) fire
    {
        NSSet* windows;
        @synchronized(self) {
            windows = [_windows copy];
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            BOOL anyDisplayed = FALSE;
            for (WineWindow* window in windows)
            {
                if ([window viewsNeedDisplay])
                {
                    [window displayIfNeeded];
                    anyDisplayed = YES;
                }
            }

            NSTimeInterval now = [[NSProcessInfo processInfo] systemUptime];
            if (anyDisplayed)
                _lastDisplayTime = now;
            else if (_lastDisplayTime + 2.0 < now)
                CVDisplayLinkStop(_link);
        });
        [windows release];
    }

    - (NSTimeInterval) refreshPeriod
    {
        if (_actualRefreshPeriod || (_actualRefreshPeriod = CVDisplayLinkGetActualOutputVideoRefreshPeriod(_link)))
            return _actualRefreshPeriod;

        if (_nominalRefreshPeriod)
            return _nominalRefreshPeriod;

        CVTime time = CVDisplayLinkGetNominalOutputVideoRefreshPeriod(_link);
        if (time.flags & kCVTimeIsIndefinite)
            return 1.0 / 60.0;
        _nominalRefreshPeriod = time.timeValue / (double)time.timeScale;
        return _nominalRefreshPeriod;
    }

    - (void) start
    {
        _lastDisplayTime = [[NSProcessInfo processInfo] systemUptime];
        CVDisplayLinkStart(_link);
    }

static CVReturn WineDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* inNow, const CVTimeStamp* inOutputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
    WineDisplayLink* link = displayLinkContext;
    [link fire];
    return kCVReturnSuccess;
}

@end


#ifndef MAC_OS_X_VERSION_10_14
@protocol NSViewLayerContentScaleDelegate <NSObject>
@optional

    - (BOOL) layer:(CALayer*)layer shouldInheritContentsScale:(CGFloat)newScale fromWindow:(NSWindow*)window;

@end
#endif


@interface CAShapeLayer (WineShapeMaskExtensions)

@property(readonly, nonatomic, getter=isEmptyShaped) BOOL emptyShaped;

@end

@implementation CAShapeLayer (WineShapeMaskExtensions)

    - (BOOL) isEmptyShaped
    {
        return CGRectEqualToRect(CGPathGetBoundingBox(self.path), CGRectZero);
    }

@end


@interface WineBaseView : NSView
@end


@interface WineMetalView : WineBaseView
{
    id<MTLDevice> _device;
}

    - (id) initWithFrame:(NSRect)frame device:(id<MTLDevice>)device;

@end


@interface WineContentView : WineBaseView <NSTextInputClient, NSViewLayerContentScaleDelegate>
{
    CGRect surfaceRect;
    CGImageRef colorImage;
    CGImageRef shapeImage;

    NSMutableArray* glContexts;
    NSMutableArray* pendingGlContexts;
    BOOL _everHadGLContext;
    BOOL _cachedHasGLDescendant;
    BOOL _cachedHasGLDescendantValid;

    NSMutableAttributedString* markedText;
    NSRange markedTextSelection;

    int backingSize[2];

    WineMetalView *_metalView;
}

@property (readonly, nonatomic) BOOL everHadGLContext;

    - (void) addGLContext:(WineOpenGLContext*)context;
    - (void) removeGLContext:(WineOpenGLContext*)context;
    - (void) updateGLContexts;

    - (void) wine_getBackingSize:(int*)outBackingSize;
    - (void) wine_setBackingSize:(const int*)newBackingSize;

    - (WineMetalView*) newMetalViewWithDevice:(id<MTLDevice>)device;

@end


@interface WineWindow ()

@property (readwrite, nonatomic) BOOL disabled;
@property (readwrite, nonatomic) BOOL noForeground;
@property (readwrite, nonatomic) BOOL preventsAppActivation;
@property (readwrite, nonatomic) BOOL floating;
@property (readwrite, nonatomic) BOOL drawnSinceShown;
@property (readwrite, nonatomic) BOOL closing;
@property (readwrite, getter=isFakingClose, nonatomic) BOOL fakingClose;
@property (retain, nonatomic) NSWindow* latentParentWindow;

@property (nonatomic) void* hwnd;
@property (retain, readwrite, nonatomic) WineEventQueue* queue;

@property (nonatomic) BOOL shapeChangedSinceLastDraw;
@property (readonly, nonatomic) BOOL needsTransparency;

@property (nonatomic) BOOL usePerPixelAlpha;

@property (assign, nonatomic) void* himc;
@property (nonatomic) BOOL commandDone;

@property (readonly, copy, nonatomic) NSArray* childWineWindows;

    - (void) setShape:(CGPathRef)newShape;

    - (void) updateForGLSubviews;

    - (BOOL) becameEligibleParentOrChild;
    - (void) becameIneligibleChild;

    - (void) windowDidDrawContent;

@end


@implementation WineBaseView

    - (void) setRetinaMode:(int)mode
    {
        for (WineBaseView* subview in [self subviews])
        {
            if ([subview isKindOfClass:[WineBaseView class]])
                [subview setRetinaMode:mode];
        }
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

    - (BOOL) mouseDownCanMoveWindow
    {
        return NO;
    }

    - (NSFocusRingType) focusRingType
    {
        return NSFocusRingTypeNone;
    }

@end


@implementation WineContentView

@synthesize everHadGLContext = _everHadGLContext;

    - (instancetype) initWithFrame:(NSRect)frame
    {
        self = [super initWithFrame:frame];
        if (self)
        {
            [self setLayerContentsPlacement:NSViewLayerContentsPlacementTopLeft];
            [self setWantsLayer:YES];
            [self setLayerRetinaProperties:retina_on];
            [self setAutoresizesSubviews:NO];
        }
        return self;
    }

    - (void) dealloc
    {
        [markedText release];
        [glContexts release];
        [pendingGlContexts release];
        CGImageRelease(colorImage);
        CGImageRelease(shapeImage);
        [super dealloc];
    }

    - (BOOL) isFlipped
    {
        return YES;
    }

    - (BOOL) wantsUpdateLayer
    {
        return YES /*!_everHadGLContext*/;
    }

    - (void) updateLayer
    {
        WineWindow* window = (WineWindow*)[self window];
        CGImageRef image, maskedImage;
        CGRect imageRect;
        CALayer* layer = [self layer];

        if ([window contentView] != self)
            return;

        if (window.closing)
            return;

        imageRect = layer.bounds;
        imageRect.origin.x *= layer.contentsScale;
        imageRect.origin.y *= layer.contentsScale;
        imageRect.size.width *= layer.contentsScale;
        imageRect.size.height *= layer.contentsScale;

        maskedImage = shapeImage ? CGImageCreateWithMask(colorImage, shapeImage)
                                 : CGImageRetain(colorImage);
        image = CGImageCreateWithImageInRect(maskedImage, imageRect);
        CGImageRelease(maskedImage);

        if (image)
        {
            layer.position = surfaceRect.origin;
            layer.contents = (id)image;
            CFRelease(image);
            [window windowDidDrawContent];

            // If the window may be transparent, then we have to invalidate the
            // shadow every time we draw.  Also, if this is the first time we've
            // drawn since changing from transparent to opaque.
            if (shapeImage || window.usePerPixelAlpha || window.shapeChangedSinceLastDraw)
            {
                window.shapeChangedSinceLastDraw = FALSE;
                [window invalidateShadow];
            }
        }
    }

    - (void) setSurfaceRect:(CGRect)rect
    {
        surfaceRect = rect;
    }

    - (void) setColorImage:(CGImageRef)image
    {
        CGImageRelease(colorImage);
        colorImage = CGImageRetain(image);
    }

    - (void) setShapeImage:(CGImageRef)image
    {
        CGImageRelease(shapeImage);
        shapeImage = CGImageRetain(image);
    }

    - (BOOL) hasShapeImage
    {
        return !!shapeImage;
    }

    - (void) viewWillDraw
    {
        [super viewWillDraw];

        for (WineOpenGLContext* context in pendingGlContexts)
        {
            context.needsUpdate = TRUE;
            macdrv_update_opengl_context((macdrv_opengl_context)context);
        }
        [glContexts addObjectsFromArray:pendingGlContexts];
        [pendingGlContexts removeAllObjects];
    }

    - (void) addGLContext:(WineOpenGLContext*)context
    {
        BOOL hadContext = _everHadGLContext;
        if (!glContexts)
            glContexts = [[NSMutableArray alloc] init];
        if (!pendingGlContexts)
            pendingGlContexts = [[NSMutableArray alloc] init];

        if ([[self window] windowNumber] > 0 && !NSIsEmptyRect([self visibleRect]))
        {
            [glContexts addObject:context];
            context.needsUpdate = TRUE;
        }
        else
        {
            [pendingGlContexts addObject:context];
            [self setNeedsDisplay:YES];
        }

        _everHadGLContext = YES;
        if (!hadContext)
            [self invalidateHasGLDescendant];
        [(WineWindow*)[self window] updateForGLSubviews];
    }

    - (void) removeGLContext:(WineOpenGLContext*)context
    {
        [glContexts removeObjectIdenticalTo:context];
        [pendingGlContexts removeObjectIdenticalTo:context];
        [(WineWindow*)[self window] updateForGLSubviews];
    }

    - (void) updateGLContexts:(BOOL)reattach
    {
        for (WineOpenGLContext* context in glContexts)
        {
            context.needsUpdate = TRUE;
            if (reattach)
                context.needsReattach = TRUE;
        }
    }

    - (void) updateGLContexts
    {
        [self updateGLContexts:NO];
    }

    - (BOOL) _hasGLDescendant
    {
        if ([self isHidden])
            return NO;
        if (_everHadGLContext)
            return YES;
        for (WineContentView* view in [self subviews])
        {
            if ([view isKindOfClass:[WineContentView class]] && [view hasGLDescendant])
                return YES;
        }
        return NO;
    }

    - (BOOL) hasGLDescendant
    {
        if (!_cachedHasGLDescendantValid)
        {
            _cachedHasGLDescendant = [self _hasGLDescendant];
            _cachedHasGLDescendantValid = YES;
        }
        return _cachedHasGLDescendant;
    }

    - (void) invalidateHasGLDescendant
    {
        BOOL invalidateAncestors = _cachedHasGLDescendantValid;
        _cachedHasGLDescendantValid = NO;
        if (invalidateAncestors && self != [[self window] contentView])
        {
            WineContentView* superview = (WineContentView*)[self superview];
            if ([superview isKindOfClass:[WineContentView class]])
                [superview invalidateHasGLDescendant];
        }
    }

    - (void) wine_getBackingSize:(int*)outBackingSize
    {
        @synchronized(self) {
            memcpy(outBackingSize, backingSize, sizeof(backingSize));
        }
    }
    - (void) wine_setBackingSize:(const int*)newBackingSize
    {
        @synchronized(self) {
            memcpy(backingSize, newBackingSize, sizeof(backingSize));
        }
    }

    - (WineMetalView*) newMetalViewWithDevice:(id<MTLDevice>)device
    {
        if (_metalView) return _metalView;

        WineMetalView* view = [[WineMetalView alloc] initWithFrame:[self bounds] device:device];
        [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        [self setAutoresizesSubviews:YES];
        [self addSubview:view positioned:NSWindowBelow relativeTo:nil];
        _metalView = view;

        [(WineWindow*)self.window windowDidDrawContent];

        return _metalView;
    }

    - (void) setLayerRetinaProperties:(int)mode
    {
        [self layer].contentsScale = mode ? 2.0 : 1.0;
        [self layer].minificationFilter = mode ? kCAFilterLinear : kCAFilterNearest;
        [self layer].magnificationFilter = mode ? kCAFilterLinear : kCAFilterNearest;

        /* On macOS 10.13 and earlier, the desired minificationFilter seems to be
         * ignored and "nearest" filtering is used, which looks terrible.
         * Enabling rasterization seems to work around this, only enable
         * it when there may be down-scaling (retina mode enabled).
         */
        if (floor(NSAppKitVersionNumber) < 1671 /*NSAppKitVersionNumber10_14*/)
        {
            if (mode)
            {
                [self layer].shouldRasterize = YES;
                [self layer].rasterizationScale = 2.0;
            }
            else
                [self layer].shouldRasterize = NO;
        }
    }

    - (void) setRetinaMode:(int)mode
    {
        double scale = mode ? 0.5 : 2.0;
        NSRect frame = self.frame;
        frame.origin.x *= scale;
        frame.origin.y *= scale;
        frame.size.width *= scale;
        frame.size.height *= scale;
        [self setFrame:frame];
        [self setWantsBestResolutionOpenGLSurface:mode];
        [self updateGLContexts];
        [self setLayerRetinaProperties:mode];

        [super setRetinaMode:mode];
    }

    - (BOOL) layer:(CALayer*)layer shouldInheritContentsScale:(CGFloat)newScale fromWindow:(NSWindow*)window
    {
        /* This method is invoked when the contentsScale of the layer is not
         * equal to the contentsScale of the window.
         * (Initially when the layer is first created, and later if the window
         * contentsScale changes, i.e. moved between retina/non-retina monitors).
         *
         * We usually want to return YES, so the "moving windows between
         * retina/non-retina monitors" case works right.
         * But return NO when we need an intentional mismatch between the
         * window and layer contentsScale
         * (non-retina mode with a retina monitor, and vice-versa).
         */
        if (layer.contentsScale != window.backingScaleFactor)
            return NO;
        return YES;
    }

    - (void) viewDidHide
    {
        [super viewDidHide];
        [self invalidateHasGLDescendant];
    }

    - (void) viewDidUnhide
    {
        [super viewDidUnhide];
        [self updateGLContexts:YES];
        [self invalidateHasGLDescendant];
    }

    - (void) clearMarkedText
    {
        [markedText deleteCharactersInRange:NSMakeRange(0, [markedText length])];
        markedTextSelection = NSMakeRange(0, 0);
        [[self inputContext] discardMarkedText];
    }

    - (void) completeText:(NSString*)text
    {
        macdrv_event* event;
        WineWindow* window = (WineWindow*)[self window];

        event = macdrv_create_event(IM_SET_TEXT, window);
        event->im_set_text.himc = [window himc];
        event->im_set_text.text = (CFStringRef)[text copy];
        event->im_set_text.complete = TRUE;

        [[window queue] postEvent:event];

        macdrv_release_event(event);

        [self clearMarkedText];
    }

    - (void) didAddSubview:(NSView*)subview
    {
        if ([subview isKindOfClass:[WineContentView class]])
        {
            WineContentView* view = (WineContentView*)subview;
            if (!view->_cachedHasGLDescendantValid || view->_cachedHasGLDescendant)
                [self invalidateHasGLDescendant];
        }
        [super didAddSubview:subview];
    }

    - (void) willRemoveSubview:(NSView*)subview
    {
        if ([subview isKindOfClass:[WineContentView class]])
        {
            WineContentView* view = (WineContentView*)subview;
            if (!view->_cachedHasGLDescendantValid || view->_cachedHasGLDescendant)
                [self invalidateHasGLDescendant];
        }
        if (subview == _metalView)
            _metalView = nil;
        [super willRemoveSubview:subview];
    }

    - (void) setLayer:(CALayer*)newLayer
    {
        [super setLayer:newLayer];
        [self updateGLContexts];
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
            event->im_set_text.himc = [window himc];
            event->im_set_text.text = (CFStringRef)[[markedText string] copy];
            event->im_set_text.complete = FALSE;
            event->im_set_text.cursor_begin = markedTextSelection.location;
            event->im_set_text.cursor_end = markedTextSelection.location + markedTextSelection.length;

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
        query->ime_char_rect.himc = [window himc];
        query->ime_char_rect.range = CFRangeMake(aRange.location, aRange.length);

        if ([window.queue query:query timeout:0.3 flags:WineQueryNoPreemptWait])
        {
            aRange = NSMakeRange(query->ime_char_rect.range.location, query->ime_char_rect.range.length);
            ret = NSRectFromCGRect(cgrect_mac_from_win(query->ime_char_rect.rect));
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


@implementation WineMetalView

    - (id) initWithFrame:(NSRect)frame device:(id<MTLDevice>)device
    {
        self = [super initWithFrame:frame];
        if (self)
        {
            _device = [device retain];
            self.wantsLayer = YES;
            self.layerContentsRedrawPolicy = NSViewLayerContentsRedrawNever;
        }
        return self;
    }

    - (void) dealloc
    {
        [_device release];
        [super dealloc];
    }

    - (void) setRetinaMode:(int)mode
    {
        self.layer.contentsScale = mode ? 2.0 : 1.0;
        [super setRetinaMode:mode];
    }

    - (CALayer*) makeBackingLayer
    {
        CAMetalLayer *layer = [CAMetalLayer layer];
        layer.device = _device;
        layer.framebufferOnly = YES;
        layer.magnificationFilter = kCAFilterNearest;
        layer.backgroundColor = CGColorGetConstantColor(kCGColorBlack);
        layer.contentsScale = retina_on ? 2.0 : 1.0;
        return layer;
    }

    - (BOOL) isOpaque
    {
        return YES;
    }

@end


@implementation WineWindow

    static WineWindow* causing_becomeKeyWindow;

    @synthesize disabled, noForeground, preventsAppActivation, floating, fullscreen, fakingClose, closing, latentParentWindow, hwnd, queue;
    @synthesize drawnSinceShown;
    @synthesize shapeChangedSinceLastDraw;
    @synthesize usePerPixelAlpha;
    @synthesize himc, commandDone;

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
        [window setDelegate:window];
        [window setBackgroundColor:[NSColor clearColor]];
        [window setOpaque:NO];
        window.hwnd = hwnd;
        window.queue = queue;
        window->savedContentMinSize = NSZeroSize;
        window->savedContentMaxSize = NSMakeSize(FLT_MAX, FLT_MAX);
        window->resizable = wf->resizable;
        window->_lastDisplayTime = [[NSDate distantPast] timeIntervalSinceReferenceDate];

        [window registerForDraggedTypes:@[(NSString*)kUTTypeData, (NSString*)kUTTypeContent]];

        contentView = [[[WineContentView alloc] initWithFrame:NSZeroRect] autorelease];
        if (!contentView)
            return nil;

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

        [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:window
                                                              selector:@selector(checkWineDisplayLink)
                                                                  name:NSWorkspaceActiveSpaceDidChangeNotification
                                                                object:[NSWorkspace sharedWorkspace]];

        [window setFrameAndWineFrame:[window frameRectForContentRect:window_frame]];

        return window;
    }

    - (void) dealloc
    {
        [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:self];
        [[NSNotificationCenter defaultCenter] removeObserver:self];
        [queue release];
        [latentChildWindows release];
        [latentParentWindow release];
        [super dealloc];
    }

    - (BOOL) preventResizing
    {
        BOOL preventForClipping = cursor_clipping_locks_windows && [[WineApplicationController sharedController] clippingCursor];
        return ([self styleMask] & NSWindowStyleMaskResizable) && (disabled || !resizable || preventForClipping);
    }

    - (BOOL) allowsMovingWithMaximized:(BOOL)inMaximized
    {
        if (allow_immovable_windows && (disabled || inMaximized))
            return NO;
        else if (cursor_clipping_locks_windows && [[WineApplicationController sharedController] clippingCursor])
            return NO;
        else
            return YES;
    }

    - (void) adjustFeaturesForState
    {
        NSUInteger style = [self styleMask];

        if (style & NSWindowStyleMaskClosable)
            [[self standardWindowButton:NSWindowCloseButton] setEnabled:!self.disabled];
        if (style & NSWindowStyleMaskMiniaturizable)
            [[self standardWindowButton:NSWindowMiniaturizeButton] setEnabled:!self.disabled];
        if (style & NSWindowStyleMaskResizable)
            [[self standardWindowButton:NSWindowZoomButton] setEnabled:!self.disabled];
        if ([self collectionBehavior] & NSWindowCollectionBehaviorFullScreenPrimary)
            [[self standardWindowButton:NSWindowFullScreenButton] setEnabled:!self.disabled];

        if ([self preventResizing])
        {
            NSSize size = [self contentRectForFrameRect:self.wine_fractionalFrame].size;
            [self setContentMinSize:size];
            [self setContentMaxSize:size];
        }
        else
        {
            [self setContentMaxSize:savedContentMaxSize];
            [self setContentMinSize:savedContentMinSize];
        }

        if (allow_immovable_windows || cursor_clipping_locks_windows)
            [self setMovable:[self allowsMovingWithMaximized:maximized]];
    }

    - (void) adjustFullScreenBehavior:(NSWindowCollectionBehavior)behavior
    {
        NSUInteger style = [self styleMask];

        if (behavior & NSWindowCollectionBehaviorParticipatesInCycle &&
            style & NSWindowStyleMaskResizable && !(style & NSWindowStyleMaskUtilityWindow) && !maximized &&
            !(self.parentWindow || self.latentParentWindow))
        {
            behavior |= NSWindowCollectionBehaviorFullScreenPrimary;
            behavior &= ~NSWindowCollectionBehaviorFullScreenAuxiliary;
        }
        else
        {
            behavior &= ~NSWindowCollectionBehaviorFullScreenPrimary;
            behavior |= NSWindowCollectionBehaviorFullScreenAuxiliary;
            if (style & NSWindowStyleMaskFullScreen)
                [super toggleFullScreen:nil];
        }

        if (behavior != [self collectionBehavior])
        {
            [self setCollectionBehavior:behavior];
            [self adjustFeaturesForState];
        }
    }

    - (void) setWindowFeatures:(const struct macdrv_window_features*)wf
    {
        static const NSUInteger usedStyles = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable |
                                             NSWindowStyleMaskResizable | NSWindowStyleMaskUtilityWindow | NSWindowStyleMaskBorderless |
                                             NSWindowStyleMaskNonactivatingPanel;
        NSUInteger currentStyle = [self styleMask];
        NSUInteger newStyle = style_mask_for_features(wf) | (currentStyle & ~usedStyles);

        self.preventsAppActivation = wf->prevents_app_activation;

        if (newStyle != currentStyle)
        {
            NSString* title = [[[self title] copy] autorelease];
            BOOL showingButtons = (currentStyle & (NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable)) != 0;
            BOOL shouldShowButtons = (newStyle & (NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable)) != 0;
            if (shouldShowButtons != showingButtons && !((newStyle ^ currentStyle) & NSWindowStyleMaskClosable))
            {
                // -setStyleMask: is buggy on 10.7+ with respect to NSWindowStyleMaskResizable.
                // If transitioning from NSWindowStyleMaskTitled | NSWindowStyleMaskResizable to
                // just NSWindowStyleMaskTitled, the window buttons should disappear rather
                // than just being disabled.  But they don't.  Similarly in reverse.
                // The workaround is to also toggle NSWindowStyleMaskClosable at the same time.
                [self setStyleMask:newStyle ^ NSWindowStyleMaskClosable];
            }
            [self setStyleMask:newStyle];

            BOOL isNonActivating = (currentStyle & NSWindowStyleMaskNonactivatingPanel) != 0;
            BOOL shouldBeNonActivating = (newStyle & NSWindowStyleMaskNonactivatingPanel) != 0;
            if (isNonActivating != shouldBeNonActivating) {
                // Changing NSWindowStyleMaskNonactivatingPanel with -setStyleMask is also
                // buggy. If it's added, clicking the title bar will still activate the
                // app. If it's removed, nothing changes at all.
                // This private method ensures the correct behavior.
                if ([self respondsToSelector:@selector(_setPreventsActivation:)])
                    [self _setPreventsActivation:shouldBeNonActivating];
            }

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
                    level = NSStatusWindowLevel + 1;

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

    - (void) sendResizeStartQuery
    {
        macdrv_query* query = macdrv_create_query();
        query->type = QUERY_RESIZE_START;
        query->window = (macdrv_window)[self retain];

        [self.queue query:query timeout:0.3];
        macdrv_release_query(query);
    }

    - (void) setMacDrvState:(const struct macdrv_window_state*)state
    {
        NSWindowCollectionBehavior behavior;

        self.disabled = state->disabled;
        self.noForeground = state->no_foreground;

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
                for (child in [self childWineWindows])
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

        if (state->minimized_valid)
        {
            macdrv_event_mask discard = event_mask_for_type(WINDOW_DID_UNMINIMIZE);

            pendingMinimize = FALSE;
            if (state->minimized && ![self isMiniaturized])
            {
                if ([self wouldBeVisible])
                {
                    if (([self styleMask] & NSWindowStyleMaskFullScreen) || stage_manager_enabled())
                    {
                        [self postDidUnminimizeEvent];
                        discard &= ~event_mask_for_type(WINDOW_DID_UNMINIMIZE);

                        /* When Stage Manager is enabled, it's not possible to minimize the window
                         * (miniaturize: just moves the window to the background).
                         * Post an unminimize event, then miniaturize:.
                         */
                        if (stage_manager_enabled())
                        {
                            [self setStyleMask:([self styleMask] | NSWindowStyleMaskMiniaturizable)];
                            [super miniaturize:nil];
                        }
                    }
                    else
                    {
                        [self setStyleMask:([self styleMask] | NSWindowStyleMaskMiniaturizable)];
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

            if (!maximized && [self inLiveResize])
                [self sendResizeStartQuery];
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
    }

    - (BOOL) addChildWineWindow:(WineWindow*)child assumeVisible:(BOOL)assumeVisible
    {
        BOOL reordered = FALSE;

        if ([self isVisible] && (assumeVisible || [child isVisible]) && (self.floating || !child.floating))
        {
            if ([self level] > [child level])
                [child setLevel:[self level]];
            if (![child isVisible])
                [child setAutodisplay:YES];
            [self addChildWindow:child ordered:NSWindowAbove];
            [child checkWineDisplayLink];
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

    - (void) setChildWineWindows:(NSArray*)childWindows
    {
        NSArray* origChildren;
        NSUInteger count, start, limit, i;

        origChildren = self.childWineWindows;

        // If the current and desired children arrays match up to a point, leave
        // those matching children alone.
        count = childWindows.count;
        limit = MIN(origChildren.count, count);
        for (start = 0; start < limit; start++)
        {
            if (origChildren[start] != childWindows[start])
                break;
        }

        // Remove all of the child windows and re-add them back-to-front so they
        // are in the desired order.
        for (i = start; i < count; i++)
        {
            WineWindow* child = childWindows[i];
            [self removeChildWindow:child];
        }
        for (i = start; i < count; i++)
        {
            WineWindow* child = childWindows[i];
            [self addChildWindow:child ordered:NSWindowAbove];
        }
    }

    static NSComparisonResult compare_windows_back_to_front(NSWindow* window1, NSWindow* window2, NSArray* windowNumbers)
    {
        NSNumber* window1Number = [NSNumber numberWithInteger:[window1 windowNumber]];
        NSNumber* window2Number = [NSNumber numberWithInteger:[window2 windowNumber]];
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
            NSMutableArray* windowNumbers;
            NSMutableArray* childWindows = [[self.childWineWindows mutableCopy] autorelease];
            NSMutableIndexSet* indexesToRemove = [NSMutableIndexSet indexSet];
            NSUInteger i;

            windowNumbers = [[[[self class] windowNumbersWithOptions:NSWindowNumberListAllSpaces] mutableCopy] autorelease];

            for (i = 0; i < count; i++)
            {
                WineWindow* child = latentChildWindows[i];
                if ([child isVisible] && (self.floating || !child.floating))
                {
                    if (child.latentParentWindow == self)
                    {
                        if ([self level] > [child level])
                            [child setLevel:[self level]];
                        [childWindows addObject:child];
                        child.latentParentWindow = nil;
                        reordered = TRUE;
                    }
                    else
                        ERR(@"shouldn't happen: %@ thinks %@ is a latent child, but it doesn't agree\n", self, child);
                    [indexesToRemove addIndex:i];
                }
            }

            [latentChildWindows removeObjectsAtIndexes:indexesToRemove];

            [childWindows sortWithOptions:NSSortStable
                          usingComparator:^NSComparisonResult(id obj1, id obj2){
                return compare_windows_back_to_front(obj1, obj2, windowNumbers);
            }];
            [self setChildWineWindows:childWindows];
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
        NSArray* childWindows = [self childWineWindows];

        [self becameIneligibleChild];

        if ([childWindows count])
        {
            WineWindow* child;

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
            NSInteger interveningWindowNumber = [windowNumbers[i] integerValue];
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
        NSUInteger otherIndex;
        NSArray* origChildren;
        NSMutableArray* children;

        // Get the z-order from the window server and modify it to reflect the
        // requested window ordering.
        windowNumbers = [[[[self class] windowNumbersWithOptions:NSWindowNumberListAllSpaces] mutableCopy] autorelease];
        childWindowNumber = [NSNumber numberWithInteger:[child windowNumber]];
        [windowNumbers removeObject:childWindowNumber];
        if (other)
        {
            otherIndex = [windowNumbers indexOfObject:[NSNumber numberWithInteger:[other windowNumber]]];
            [windowNumbers insertObject:childWindowNumber atIndex:otherIndex + (mode == NSWindowAbove ? 0 : 1)];
        }
        else if (mode == NSWindowAbove)
            [windowNumbers insertObject:childWindowNumber atIndex:0];
        else
            [windowNumbers addObject:childWindowNumber];

        // Get our child windows and sort them in the reverse of the desired
        // z-order (back-to-front).
        origChildren = [self childWineWindows];
        children = [[origChildren mutableCopy] autorelease];
        [children sortWithOptions:NSSortStable
                  usingComparator:^NSComparisonResult(id obj1, id obj2){
            return compare_windows_back_to_front(obj1, obj2, windowNumbers);
        }];

        [self setChildWineWindows:children];
    }

    // Search the ancestor windows of self and other to find a place where some ancestors are siblings of each other.
    // There are three possible results in terms of the values of *ancestor and *ancestorOfOther on return:
    //      (non-nil, non-nil)  there is a level in the window tree where the two windows have sibling ancestors
    //                          if *ancestor has a parent Wine window, then it's the parent of the other ancestor, too
    //                          otherwise, the two ancestors are each roots of disjoint window trees
    //      (nil, non-nil)      the other window is a descendent of self and *ancestorOfOther is the direct child
    //      (non-nil, nil)      self is a descendent of other and *ancestor is the direct child
    - (void) getSiblingWindowsForWindow:(WineWindow*)other ancestor:(WineWindow**)ancestor ancestorOfOther:(WineWindow**)ancestorOfOther
    {
        NSMutableArray* otherAncestors = [NSMutableArray arrayWithObject:other];
        WineWindow* child;
        WineWindow* parent;
        for (child = other;
             (parent = (WineWindow*)child.parentWindow) && [parent isKindOfClass:[WineWindow class]];
             child = parent)
        {
            if (parent == self)
            {
                *ancestor = nil;
                *ancestorOfOther = child;
                return;
            }

            [otherAncestors addObject:parent];
        }

        for (child = self;
             (parent = (WineWindow*)child.parentWindow) && [parent isKindOfClass:[WineWindow class]];
             child = parent)
        {
            NSUInteger index = [otherAncestors indexOfObjectIdenticalTo:parent];
            if (index != NSNotFound)
            {
                *ancestor = child;
                if (index == 0)
                    *ancestorOfOther = nil;
                else
                    *ancestorOfOther = otherAncestors[index - 1];
                return;
            }
        }

        *ancestor = child;
        *ancestorOfOther = otherAncestors.lastObject;;
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
            WineWindow* parent;
            WineWindow* child;

            [controller transformProcessToForeground:!self.preventsAppActivation];
            if ([NSApp isHidden])
                [NSApp unhide:nil];
            wasVisible = [self isVisible];

            if (activate)
                [controller tryToActivateIgnoringOtherApps:YES];

            NSDisableScreenUpdates();

            if ([self becameEligibleParentOrChild])
                needAdjustWindowLevels = TRUE;

            if (prev || next)
            {
                WineWindow* other = [prev isVisible] ? prev : next;
                NSWindowOrderingMode orderingMode = [prev isVisible] ? NSWindowBelow : NSWindowAbove;

                if (![self isOrdered:orderingMode relativeTo:other])
                {
                    WineWindow* ancestor;
                    WineWindow* ancestorOfOther;

                    [self getSiblingWindowsForWindow:other ancestor:&ancestor ancestorOfOther:&ancestorOfOther];
                    if (ancestor)
                    {
                        [self setAutodisplay:YES];
                        if (ancestorOfOther)
                        {
                            // This window level may not be right for this window based
                            // on floating-ness, fullscreen-ness, etc.  But we set it
                            // temporarily to allow us to order the windows properly.
                            // Then the levels get fixed by -adjustWindowLevels.
                            if ([ancestor level] != [ancestorOfOther level])
                                [ancestor setLevel:[ancestorOfOther level]];

                            parent = (WineWindow*)ancestor.parentWindow;
                            if ([parent isKindOfClass:[WineWindow class]])
                                [parent order:orderingMode childWindow:ancestor relativeTo:ancestorOfOther];
                            else
                                [ancestor orderWindow:orderingMode relativeTo:[ancestorOfOther windowNumber]];
                        }

                        if (!ancestorOfOther || ancestor != self)
                        {
                            for (child = self;
                                 (parent = (WineWindow*)child.parentWindow);
                                 child = parent)
                            {
                                if ([parent isKindOfClass:[WineWindow class]])
                                    [parent order:-orderingMode childWindow:child relativeTo:nil];
                                if (parent == ancestor)
                                    break;
                            }
                        }

                        [self checkWineDisplayLink];
                        needAdjustWindowLevels = TRUE;
                    }
                }
            }
            else
            {
                for (child = self;
                     (parent = (WineWindow*)child.parentWindow) && [parent isKindOfClass:[WineWindow class]];
                     child = parent)
                {
                    [parent order:NSWindowAbove childWindow:child relativeTo:nil];
                }

                // Again, temporarily set level to make sure we can order to
                // the right place.
                next = [controller frontWineWindow];
                if (next && [self level] < [next level])
                    [self setLevel:[next level]];
                [self setAutodisplay:YES];
                [self orderFront:nil];
                [self checkWineDisplayLink];
                needAdjustWindowLevels = TRUE;
            }
            pendingOrderOut = FALSE;

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
                [self setStyleMask:([self styleMask] | NSWindowStyleMaskMiniaturizable)];
                [super miniaturize:nil];
                pendingMinimize = FALSE;
            }

            NSEnableScreenUpdates();

            /* Cocoa may adjust the frame when the window is ordered onto the screen.
               Generate a frame-changed event just in case.  The back end will ignore
               it if nothing actually changed. */
            [self windowDidResize:nil skipSizeMove:TRUE];

            if (![self isExcludedFromWindowsMenu])
                [NSApp addWindowsItem:self title:[self title] filename:NO];
        }
    }

    - (void) doOrderOut
    {
        WineApplicationController* controller = [WineApplicationController sharedController];
        BOOL wasVisible = [self isVisible];
        BOOL wasOnActiveSpace = [self isOnActiveSpace];

        [self endWindowDragging];
        [controller windowWillOrderOut:self];

        if (enteringFullScreen || exitingFullScreen)
        {
            pendingOrderOut = TRUE;
            [queue discardEventsMatchingMask:event_mask_for_type(WINDOW_BROUGHT_FORWARD) |
                                             event_mask_for_type(WINDOW_GOT_FOCUS) |
                                             event_mask_for_type(WINDOW_LOST_FOCUS) |
                                             event_mask_for_type(WINDOW_MAXIMIZE_REQUESTED) |
                                             event_mask_for_type(WINDOW_MINIMIZE_REQUESTED) |
                                             event_mask_for_type(WINDOW_RESTORE_REQUESTED)
                                   forWindow:self];
            return;
        }

        pendingOrderOut = FALSE;

        if ([self isMiniaturized])
            pendingMinimize = TRUE;

        WineWindow* parent = (WineWindow*)self.parentWindow;
        if ([parent isKindOfClass:[WineWindow class]])
            [parent grabDockIconSnapshotFromWindow:self force:NO];

        [self becameIneligibleParentOrChild];
        if ([self isMiniaturized] || [self styleMask] & NSWindowStyleMaskFullScreen)
        {
            fakingClose = TRUE;
            [self close];
            fakingClose = FALSE;
        }
        else
            [self orderOut:nil];
        [self checkWineDisplayLink];
        [self setBackgroundColor:[NSColor clearColor]];
        [self setOpaque:NO];
        drawnSinceShown = NO;
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
        NSRect contentRect = [self contentRectForFrameRect:self.wine_fractionalFrame];
        BOOL nowFullscreen = !([self styleMask] & NSWindowStyleMaskFullScreen) && screen_covered_by_rect(contentRect, [NSScreen screens]);

        if (nowFullscreen != fullscreen)
        {
            WineApplicationController* controller = [WineApplicationController sharedController];

            fullscreen = nowFullscreen;
            if ([self isVisible] && [self isOnActiveSpace])
                [controller updateFullscreenWindows];

            [controller adjustWindowLevels];
        }
    }

    - (void) setFrameAndWineFrame:(NSRect)frame
    {
        [self setFrame:frame display:YES];

        wineFrame = frame;
        roundedWineFrame = self.frame;
        CGFloat junk;
#if CGFLOAT_IS_DOUBLE
        if ((!modf(wineFrame.origin.x, &junk) && !modf(wineFrame.origin.y, &junk) &&
             !modf(wineFrame.size.width, &junk) && !modf(wineFrame.size.height, &junk)) ||
            fabs(wineFrame.origin.x - roundedWineFrame.origin.x) >= 1 ||
            fabs(wineFrame.origin.y - roundedWineFrame.origin.y) >= 1 ||
            fabs(wineFrame.size.width - roundedWineFrame.size.width) >= 1 ||
            fabs(wineFrame.size.height - roundedWineFrame.size.height) >= 1)
            roundedWineFrame = wineFrame;
#else
        if ((!modff(wineFrame.origin.x, &junk) && !modff(wineFrame.origin.y, &junk) &&
             !modff(wineFrame.size.width, &junk) && !modff(wineFrame.size.height, &junk)) ||
            fabsf(wineFrame.origin.x - roundedWineFrame.origin.x) >= 1 ||
            fabsf(wineFrame.origin.y - roundedWineFrame.origin.y) >= 1 ||
            fabsf(wineFrame.size.width - roundedWineFrame.size.width) >= 1 ||
            fabsf(wineFrame.size.height - roundedWineFrame.size.height) >= 1)
            roundedWineFrame = wineFrame;
#endif
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

            oldFrame = self.wine_fractionalFrame;
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

                if (equalSizes && [[self childWineWindows] count])
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

                [self setFrameAndWineFrame:frame];
                if ([self preventResizing])
                {
                    [self setContentMinSize:contentRect.size];
                    [self setContentMaxSize:contentRect.size];
                }

                if (needEnableScreenUpdates)
                    NSEnableScreenUpdates();

                if (!enteringFullScreen &&
                    [[NSProcessInfo processInfo] systemUptime] - enteredFullScreenTime > 1.0)
                    nonFullscreenFrame = frame;

                [self updateFullscreen];

                if ([self isOrderedIn])
                {
                    /* In case Cocoa adjusted the frame we tried to set, generate a frame-changed
                       event.  The back end will ignore it if nothing actually changed. */
                    [self windowDidResize:nil skipSizeMove:TRUE];
                }
            }
        }
    }

    - (NSRect) wine_fractionalFrame
    {
        NSRect frame = self.frame;
        if (NSEqualRects(frame, roundedWineFrame))
            frame = wineFrame;
        return frame;
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
            [self adjustFullScreenBehavior:[self collectionBehavior]];
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
        WineContentView *view = self.contentView;
        return self.contentView.layer.mask || [view hasShapeImage] || self.usePerPixelAlpha ||
                (gl_surface_mode == GL_SURFACE_BEHIND && [view hasGLDescendant]);
    }

    - (void) checkTransparency
    {
        if (![self isOpaque] && !self.needsTransparency)
        {
            self.shapeChangedSinceLastDraw = TRUE;
            [[self contentView] setNeedsDisplay:YES];
            [self setBackgroundColor:[NSColor windowBackgroundColor]];
            [self setOpaque:YES];
        }
        else if ([self isOpaque] && self.needsTransparency)
        {
            self.shapeChangedSinceLastDraw = TRUE;
            [[self contentView] setNeedsDisplay:YES];
            [self setBackgroundColor:[NSColor clearColor]];
            [self setOpaque:NO];
        }
    }

    - (void) setShape:(CGPathRef)newShape
    {
        CALayer* layer = [[self contentView] layer];
        CAShapeLayer* mask = (CAShapeLayer*)layer.mask;
        if (CGPathEqualToPath(newShape, mask.path)) return;

        if (newShape && !layer.mask)
            layer.mask = mask = [CAShapeLayer layer];
        else if (!newShape)
            layer.mask = mask = nil;

        if (mask.path)
            [[self contentView] setNeedsDisplayInRect:NSRectFromCGRect(CGPathGetBoundingBox(mask.path))];
        if (newShape)
            [[self contentView] setNeedsDisplayInRect:NSRectFromCGRect(CGPathGetBoundingBox(newShape))];

        mask.path = newShape;
        self.shapeChangedSinceLastDraw = TRUE;

        [self checkTransparency];
        [self checkEmptyShaped];
    }

    - (void) makeFocused:(BOOL)activate
    {
        if (activate)
        {
            WineApplicationController *controller = [WineApplicationController sharedController];
            [controller transformProcessToForeground:YES];
            [controller tryToActivateIgnoringOtherApps:YES];
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
            controller.keyboardType = CGEventGetIntegerValueField(cgevent, kCGKeyboardEventKeyboardType);

        [queue postEvent:event];

        macdrv_release_event(event);

        [controller noteKey:keyCode pressed:pressed];
    }

    - (void) postKeyEvent:(NSEvent *)theEvent
    {
        [self flagsChanged:theEvent];
        [self postKey:[theEvent keyCode]
              pressed:[theEvent type] == NSEventTypeKeyDown
            modifiers:adjusted_modifiers_for_settings([theEvent modifierFlags])
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

    - (void) postWindowFrameChanged:(NSRect)frame fullscreen:(BOOL)isFullscreen resizing:(BOOL)resizing skipSizeMove:(BOOL)skipSizeMove
    {
        macdrv_event* event;
        NSUInteger style = self.styleMask;

        if (isFullscreen)
            style |= NSWindowStyleMaskFullScreen;
        else
            style &= ~NSWindowStyleMaskFullScreen;
        frame = [[self class] contentRectForFrameRect:frame styleMask:style];
        [[WineApplicationController sharedController] flipRect:&frame];

        /* Coalesce events by discarding any previous ones still in the queue. */
        [queue discardEventsMatchingMask:event_mask_for_type(WINDOW_FRAME_CHANGED)
                               forWindow:self];

        event = macdrv_create_event(WINDOW_FRAME_CHANGED, self);
        event->window_frame_changed.frame = cgrect_win_from_mac(NSRectToCGRect(frame));
        event->window_frame_changed.fullscreen = isFullscreen;
        event->window_frame_changed.in_resize = resizing;
        event->window_frame_changed.skip_size_move_loop = skipSizeMove;
        [queue postEvent:event];
        macdrv_release_event(event);
    }

    - (void) updateForCursorClipping
    {
        [self adjustFeaturesForState];
    }

    - (void) endWindowDragging
    {
        if (draggingPhase)
        {
            if (draggingPhase == 3)
            {
                macdrv_event* event = macdrv_create_event(WINDOW_DRAG_END, self);
                [queue postEvent:event];
                macdrv_release_event(event);
            }

            draggingPhase = 0;
            [[WineApplicationController sharedController] window:self isBeingDragged:NO];
        }
    }

    - (NSMutableDictionary*) displayIDToDisplayLinkMap
    {
        static NSMutableDictionary* displayIDToDisplayLinkMap;
        if (!displayIDToDisplayLinkMap)
        {
            displayIDToDisplayLinkMap = [[NSMutableDictionary alloc] init];

            [[NSNotificationCenter defaultCenter] addObserverForName:NSApplicationDidChangeScreenParametersNotification
                                                              object:NSApp
                                                               queue:nil
                                                          usingBlock:^(NSNotification *note){
                NSMutableSet* badDisplayIDs = [NSMutableSet setWithArray:displayIDToDisplayLinkMap.allKeys];
                NSSet* validDisplayIDs = [NSSet setWithArray:[[NSScreen screens] valueForKeyPath:@"deviceDescription.NSScreenNumber"]];
                [badDisplayIDs minusSet:validDisplayIDs];
                [displayIDToDisplayLinkMap removeObjectsForKeys:[badDisplayIDs allObjects]];
            }];
        }
        return displayIDToDisplayLinkMap;
    }

    - (WineDisplayLink*) wineDisplayLink
    {
        if (!_lastDisplayID)
            return nil;

        return [self displayIDToDisplayLinkMap][@(_lastDisplayID)];
    }

    - (void) checkWineDisplayLink
    {
        NSScreen* screen = self.screen;
        if (![self isVisible] || ![self isOnActiveSpace] || [self isMiniaturized] || [self isEmptyShaped])
            screen = nil;
        if (!(self.occlusionState & NSWindowOcclusionStateVisible))
            screen = nil;

        NSNumber* displayIDNumber = screen.deviceDescription[@"NSScreenNumber"];
        CGDirectDisplayID displayID = [displayIDNumber unsignedIntValue];
        if (displayID == _lastDisplayID)
            return;

        NSMutableDictionary* displayIDToDisplayLinkMap = [self displayIDToDisplayLinkMap];

        if (_lastDisplayID)
        {
            WineDisplayLink* link = displayIDToDisplayLinkMap[@(_lastDisplayID)];
            [link removeWindow:self];
        }
        if (displayID)
        {
            WineDisplayLink* link = displayIDToDisplayLinkMap[displayIDNumber];
            if (!link)
            {
                link = [[[WineDisplayLink alloc] initWithDisplayID:displayID] autorelease];
                [displayIDToDisplayLinkMap setObject:link forKey:displayIDNumber];
            }
            [link addWindow:self];
            [self displayIfNeeded];
        }
        _lastDisplayID = displayID;
    }

    - (BOOL) isEmptyShaped
    {
        CAShapeLayer* mask = (CAShapeLayer*)[[self contentView] layer].mask;
        return ([mask isEmptyShaped]);
    }

    - (BOOL) presentsVisibleContent
    {
        if (NSWidth(self.frame) > 0 && NSHeight(self.frame) > 0 && ![self isEmptyShaped])
            return YES;

        for (WineWindow *child in self.childWindows)
        {
            if ([child isKindOfClass:[WineWindow class]] && [child presentsVisibleContent])
                return YES;
        }

        return NO;
    }

    - (BOOL) canProvideSnapshot
    {
        return (self.windowNumber > 0 && ![self isEmptyShaped]);
    }

    - (void) grabDockIconSnapshotFromWindow:(WineWindow*)window force:(BOOL)force
    {
        if (![self isEmptyShaped])
            return;

        NSTimeInterval now = [[NSProcessInfo processInfo] systemUptime];
        if (!force && now < lastDockIconSnapshot + 1)
            return;

        if (window)
        {
            if (![window canProvideSnapshot])
                return;
        }
        else
        {
            CGFloat bestArea;
            for (WineWindow* childWindow in self.childWindows)
            {
                if (![childWindow isKindOfClass:[WineWindow class]] || ![childWindow canProvideSnapshot])
                    continue;

                NSSize size = childWindow.frame.size;
                CGFloat area = size.width * size.height;
                if (!window || area > bestArea)
                {
                    window = childWindow;
                    bestArea = area;
                }
            }

            if (!window)
                return;
        }

        static CGImageRef __nullable (*pCGWindowListCreateImageFromArray)(CGRect, CFArrayRef, CGWindowImageOption);
        static dispatch_once_t once;
        dispatch_once(&once, ^{
            void *h = dlopen("/System/Library/Frameworks/CoreGraphics.framework/CoreGraphics", RTLD_LAZY | RTLD_LOCAL);
            if (h)
                pCGWindowListCreateImageFromArray = dlsym(h, "CGWindowListCreateImageFromArray");
        });

        if (!pCGWindowListCreateImageFromArray)
            return;

        const void* windowID = (const void*)(uintptr_t)(CGWindowID)window.windowNumber;
        CFArrayRef windowIDs = CFArrayCreate(NULL, &windowID, 1, NULL);
        CGImageRef windowImage = pCGWindowListCreateImageFromArray(CGRectNull, windowIDs, kCGWindowImageBoundsIgnoreFraming);
        CFRelease(windowIDs);
        if (!windowImage)
            return;

        NSImage* appImage = [NSApp applicationIconImage];
        if (!appImage)
            appImage = [NSImage imageNamed:NSImageNameApplicationIcon];

        NSImage* dockIcon = [[[NSImage alloc] initWithSize:NSMakeSize(256, 256)] autorelease];
        [dockIcon lockFocus];

        CGContextRef cgcontext = [[NSGraphicsContext currentContext] graphicsPort];

        CGRect rect = CGRectMake(8, 8, 240, 240);
        size_t width = CGImageGetWidth(windowImage);
        size_t height = CGImageGetHeight(windowImage);
        if (width > height)
        {
            rect.size.height *= height / (double)width;
            rect.origin.y += (CGRectGetWidth(rect) - CGRectGetHeight(rect)) / 2;
        }
        else if (width != height)
        {
            rect.size.width *= width / (double)height;
            rect.origin.x += (CGRectGetHeight(rect) - CGRectGetWidth(rect)) / 2;
        }

        CGContextDrawImage(cgcontext, rect, windowImage);
        [appImage drawInRect:NSMakeRect(156, 4, 96, 96)
                    fromRect:NSZeroRect
                   operation:NSCompositingOperationSourceOver
                    fraction:1
              respectFlipped:YES
                       hints:nil];

        [dockIcon unlockFocus];

        CGImageRelease(windowImage);

        NSImageView* imageView = (NSImageView*)self.dockTile.contentView;
        if (![imageView isKindOfClass:[NSImageView class]])
        {
            imageView = [[[NSImageView alloc] initWithFrame:NSMakeRect(0, 0, 256, 256)] autorelease];
            imageView.imageScaling = NSImageScaleProportionallyUpOrDown;
            self.dockTile.contentView = imageView;
        }
        imageView.image = dockIcon;
        [self.dockTile display];
        lastDockIconSnapshot = now;
    }

    - (void) checkEmptyShaped
    {
        if (self.dockTile.contentView && ![self isEmptyShaped])
        {
            self.dockTile.contentView = nil;
            lastDockIconSnapshot = 0;
        }
        [self checkWineDisplayLink];
    }


    /*
     * ---------- NSWindow method overrides ----------
     */
    - (BOOL) canBecomeKeyWindow
    {
        if (causing_becomeKeyWindow == self) return YES;
        if (self.disabled || self.noForeground) return NO;
        if ([self isKeyWindow]) return YES;

        // If a window's collectionBehavior says it participates in cycling,
        // it must return YES from this method to actually be eligible.
        return ![self isExcludedFromWindowsMenu];
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

    // This private method of NSWindow is called as Cocoa reacts to the display
    // configuration changing.  Among other things, it adjusts the window's
    // frame based on how the screen(s) changed size.  That tells Wine that the
    // window has been moved.  We don't want that.  Rather, we want to make
    // sure that the WinAPI notion of the window position is maintained/
    // restored, possibly undoing or overriding Cocoa's adjustment.
    //
    // So, we queue a REASSERT_WINDOW_POSITION event to the back end before
    // Cocoa has a chance to adjust the frame, thus preceding any resulting
    // WINDOW_FRAME_CHANGED event that may get queued.  The back end will
    // reassert its notion of the position.  That call won't get processed
    // until after this method returns, so it will override whatever this
    // method does to the window position.  It will also discard any pending
    // WINDOW_FRAME_CHANGED events.
    //
    // Unfortunately, the only way I've found to know when Cocoa is _about to_
    // adjust the window's position due to a display change is to hook into
    // this private method.  This private method has remained stable from 10.6
    // through 10.11.  If it does change, the most likely thing is that it
    // will be removed and no longer called and this fix will simply stop
    // working.  The only real danger would be if Apple changed the return type
    // to a struct or floating-point type, which would change the calling
    // convention.
    - (id) _displayChanged
    {
        macdrv_event* event = macdrv_create_event(REASSERT_WINDOW_POSITION, self);
        [queue postEvent:event];
        macdrv_release_event(event);

        return [super _displayChanged];
    }

    - (BOOL) isExcludedFromWindowsMenu
    {
        return !([self collectionBehavior] & NSWindowCollectionBehaviorParticipatesInCycle);
    }

    - (BOOL) validateMenuItem:(NSMenuItem *)menuItem
    {
        BOOL ret = [super validateMenuItem:menuItem];

        if ([menuItem action] == @selector(makeKeyAndOrderFront:))
            ret = [self isKeyWindow] || (!self.disabled && !self.noForeground);
        if ([menuItem action] == @selector(toggleFullScreen:) && (self.disabled || maximized))
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

        if (![self isKeyWindow] && !self.disabled && !self.noForeground)
            [[WineApplicationController sharedController] windowGotFocus:self];
    }

    - (void) sendEvent:(NSEvent*)event
    {
        NSEventType type = event.type;

        /* NSWindow consumes certain key-down events as part of Cocoa's keyboard
           interface control.  For example, Control-Tab switches focus among
           views.  We want to bypass that feature, so directly route key-down
           events to -keyDown:. */
        if (type == NSEventTypeKeyDown)
            [[self firstResponder] keyDown:event];
        else
        {
            if (!draggingPhase && maximized && ![self isMovable] &&
                ![self allowsMovingWithMaximized:YES] && [self allowsMovingWithMaximized:NO] &&
                type == NSEventTypeLeftMouseDown && (self.styleMask & NSWindowStyleMaskTitled))
            {
                NSRect titleBar = self.frame;
                NSRect contentRect = [self contentRectForFrameRect:titleBar];
                titleBar.size.height = NSMaxY(titleBar) - NSMaxY(contentRect);
                titleBar.origin.y = NSMaxY(contentRect);

                dragStartPosition = [self convertBaseToScreen:event.locationInWindow];

                if (NSMouseInRect(dragStartPosition, titleBar, NO))
                {
                    static const NSWindowButton buttons[] = {
                        NSWindowCloseButton,
                        NSWindowMiniaturizeButton,
                        NSWindowZoomButton,
                        NSWindowFullScreenButton,
                    };
                    BOOL hitButton = NO;
                    int i;

                    for (i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++)
                    {
                        NSButton* button = [self standardWindowButton:buttons[i]];
                        if ([button hitTest:[button.superview convertPoint:event.locationInWindow fromView:nil]])
                        {
                            hitButton = YES;
                            break;
                        }
                    }

                    if (!hitButton)
                    {
                        draggingPhase = 1;
                        dragWindowStartPosition = NSMakePoint(NSMinX(titleBar), NSMaxY(titleBar));
                        [[WineApplicationController sharedController] window:self isBeingDragged:YES];
                    }
                }
            }
            else if (draggingPhase && (type == NSEventTypeLeftMouseDragged || type == NSEventTypeLeftMouseUp))
            {
                if ([self isMovable])
                {
                    NSPoint point = [self convertBaseToScreen:event.locationInWindow];
                    NSPoint newTopLeft = dragWindowStartPosition;

                    newTopLeft.x += point.x - dragStartPosition.x;
                    newTopLeft.y += point.y - dragStartPosition.y;

                    if (draggingPhase == 2)
                    {
                        macdrv_event* mevent = macdrv_create_event(WINDOW_DRAG_BEGIN, self);
                        mevent->window_drag_begin.no_activate = [event wine_commandKeyDown];
                        [queue postEvent:mevent];
                        macdrv_release_event(mevent);

                        draggingPhase = 3;
                    }

                    [self setFrameTopLeftPoint:newTopLeft];
                }
                else if (draggingPhase == 1 && type == NSEventTypeLeftMouseDragged)
                {
                    macdrv_event* event;
                    NSRect frame = [self contentRectForFrameRect:self.frame];

                    [[WineApplicationController sharedController] flipRect:&frame];

                    event = macdrv_create_event(WINDOW_RESTORE_REQUESTED, self);
                    event->window_restore_requested.keep_frame = TRUE;
                    event->window_restore_requested.frame = cgrect_win_from_mac(NSRectToCGRect(frame));
                    [queue postEvent:event];
                    macdrv_release_event(event);

                    draggingPhase = 2;
                }

                if (type == NSEventTypeLeftMouseUp)
                    [self endWindowDragging];
            }

            [super sendEvent:event];
        }
    }

    - (void) miniaturize:(id)sender
    {
        /* When Stage Manager is enabled, miniaturize: just moves the app/window to
         * the background rather than minimizing the window.
         * Don't start minimizing the window on the Win32 side.
         */
        if (stage_manager_enabled())
        {
            [super miniaturize:sender];
            return;
        }

        macdrv_event* event = macdrv_create_event(WINDOW_MINIMIZE_REQUESTED, self);
        [queue postEvent:event];
        macdrv_release_event(event);

        WineWindow* parent = (WineWindow*)self.parentWindow;
        if ([parent isKindOfClass:[WineWindow class]])
            [parent grabDockIconSnapshotFromWindow:self force:YES];
    }

    - (void) toggleFullScreen:(id)sender
    {
        if (!self.disabled && !maximized)
            [super toggleFullScreen:sender];
    }

    - (void) setViewsNeedDisplay:(BOOL)value
    {
        if (value && ![self viewsNeedDisplay])
        {
            WineDisplayLink* link = [self wineDisplayLink];
            if (link)
            {
                NSTimeInterval now = [[NSProcessInfo processInfo] systemUptime];
                if (_lastDisplayTime + [link refreshPeriod] < now)
                    [self setAutodisplay:YES];
                else
                {
                    [link start];
                    _lastDisplayTime = now;
                }
            }
            else
                [self setAutodisplay:YES];
        }
        [super setViewsNeedDisplay:value];
    }

    - (void) display
    {
        _lastDisplayTime = [[NSProcessInfo processInfo] systemUptime];
        [super display];
        if (_lastDisplayID)
            [self setAutodisplay:NO];
    }

    - (void) displayIfNeeded
    {
        _lastDisplayTime = [[NSProcessInfo processInfo] systemUptime];
        [super displayIfNeeded];
        if (_lastDisplayID)
            [self setAutodisplay:NO];
    }

    - (void) setFrame:(NSRect)frameRect display:(BOOL)flag
    {
        if (flag)
            [self setAutodisplay:YES];
        [super setFrame:frameRect display:flag];
    }

    - (void) setFrame:(NSRect)frameRect display:(BOOL)displayFlag animate:(BOOL)animateFlag
    {
        if (displayFlag)
            [self setAutodisplay:YES];
        [super setFrame:frameRect display:displayFlag animate:animateFlag];
    }

    - (void) windowDidDrawContent
    {
        if (!drawnSinceShown)
        {
            drawnSinceShown = YES;
            dispatch_async(dispatch_get_main_queue(), ^{
                [self checkTransparency];
            });
        }
    }

    - (NSArray*) childWineWindows
    {
        NSArray* childWindows = self.childWindows;
        NSIndexSet* indexes = [childWindows indexesOfObjectsPassingTest:^BOOL(id child, NSUInteger idx, BOOL *stop){
            return [child isKindOfClass:[WineWindow class]];
        }];
        return [childWindows objectsAtIndexes:indexes];
    }

    - (void) updateForGLSubviews
    {
        if (gl_surface_mode == GL_SURFACE_BEHIND)
            [self checkTransparency];
    }

    - (void) setRetinaMode:(int)mode
    {
        NSRect frame;
        double scale = mode ? 0.5 : 2.0;
        NSAffineTransform* transform = [NSAffineTransform transform];

        [transform scaleBy:scale];

        [[self contentView] layer].mask.contentsScale = mode ? 2.0 : 1.0;

        for (WineBaseView* subview in [self.contentView subviews])
        {
            if ([subview isKindOfClass:[WineBaseView class]])
                [subview setRetinaMode:mode];
        }

        frame = [self contentRectForFrameRect:self.wine_fractionalFrame];
        frame.origin.x *= scale;
        frame.origin.y *= scale;
        frame.size.width *= scale;
        frame.size.height *= scale;
        frame = [self frameRectForContentRect:frame];

        savedContentMinSize = [transform transformSize:savedContentMinSize];
        if (savedContentMaxSize.width != FLT_MAX && savedContentMaxSize.width != CGFLOAT_MAX)
            savedContentMaxSize.width *= scale;
        if (savedContentMaxSize.height != FLT_MAX && savedContentMaxSize.height != CGFLOAT_MAX)
            savedContentMaxSize.height *= scale;

        self.contentMinSize = [transform transformSize:self.contentMinSize];
        NSSize temp = self.contentMaxSize;
        if (temp.width != FLT_MAX && temp.width != CGFLOAT_MAX)
            temp.width *= scale;
        if (temp.height != FLT_MAX && temp.height != CGFLOAT_MAX)
            temp.height *= scale;
        self.contentMaxSize = temp;

        ignore_windowResize = TRUE;
        [self setFrameAndWineFrame:frame];
        ignore_windowResize = FALSE;
    }

    - (void) setMask:(CGRect)rect
    {
        /* Draw black bars to cover every part of the window except for 'rect'.
         * Intended for use on a window covering the full screen.
         */
        if (CGRectIsEmpty(rect))
        {
            [[[self.contentView.layer sublayers] firstObject] removeFromSuperlayer];
            return;
        }

        CAShapeLayer *shapeLayer = [CAShapeLayer layer];
        shapeLayer.bounds = self.contentView.layer.bounds;
        shapeLayer.position = self.contentView.layer.position;
        shapeLayer.geometryFlipped = self.contentView.layer.geometryFlipped;
        shapeLayer.anchorPoint = self.contentView.layer.anchorPoint;
        shapeLayer.fillColor = CGColorGetConstantColor(kCGColorBlack);

        CGMutablePathRef path = CGPathCreateMutable();
        /* left/right */
        if (rect.origin.x > 0.0)
            CGPathAddRect(path, NULL, CGRectMake(0, 0, rect.origin.x, shapeLayer.bounds.size.height));
        if (rect.origin.x + rect.size.width < shapeLayer.bounds.size.width)
            CGPathAddRect(path, NULL, CGRectMake(rect.origin.x + rect.size.width,
                                                 0,
                                                 shapeLayer.bounds.size.width - (rect.origin.x + rect.size.width),
                                                 shapeLayer.bounds.size.height));

        /* top/bottom */
        if (rect.origin.y > 0.0)
            CGPathAddRect(path, NULL, CGRectMake(0, 0, shapeLayer.bounds.size.width, rect.origin.y));
        if (rect.origin.y + rect.size.height < shapeLayer.bounds.size.height)
            CGPathAddRect(path, NULL, CGRectMake(0,
                                                 rect.origin.y + rect.size.height,
                                                 shapeLayer.bounds.size.width,
                                                 shapeLayer.bounds.size.height - (rect.origin.y + rect.size.height)));

        shapeLayer.path = path;
        CGPathRelease(path);

        if ([[self.contentView.layer sublayers] firstObject])
            [self.contentView.layer replaceSublayer:[[self.contentView.layer sublayers] firstObject]
                                               with:shapeLayer];
        else
            [self.contentView.layer addSublayer:shapeLayer];
    }


    /*
     * ---------- NSResponder method overrides ----------
     */
    - (void) keyDown:(NSEvent *)theEvent
    {
        if ([theEvent isARepeat])
        {
            if (!allowKeyRepeats)
                return;
        }
        else
            allowKeyRepeats = YES;

        [self postKeyEvent:theEvent];
    }

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

        NSUInteger modifierFlags = adjusted_modifiers_for_settings([theEvent modifierFlags]);
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

                if (pressed)
                    allowKeyRepeats = NO;

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

    - (void) windowDidChangeOcclusionState:(NSNotification*)notification
    {
        [self checkWineDisplayLink];
    }

    - (void) windowDidChangeScreen:(NSNotification*)notification
    {
        [self checkWineDisplayLink];
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

        if (!self.disabled && !self.noForeground)
        {
            causing_becomeKeyWindow = self;
            [self makeKeyWindow];
            causing_becomeKeyWindow = nil;
            [controller windowGotFocus:self];
        }

        [self windowDidResize:notification];
        [self checkWineDisplayLink];
    }

    - (void) windowDidEndLiveResize:(NSNotification *)notification
    {
        if (!maximized)
        {
            macdrv_event* event = macdrv_create_event(WINDOW_RESIZE_ENDED, self);
            [queue postEvent:event];
            macdrv_release_event(event);
        }
    }

    - (void) windowDidEnterFullScreen:(NSNotification*)notification
    {
        enteringFullScreen = FALSE;
        enteredFullScreenTime = [[NSProcessInfo processInfo] systemUptime];
        if (pendingOrderOut)
            [self doOrderOut];
    }

    - (void) windowDidExitFullScreen:(NSNotification*)notification
    {
        exitingFullScreen = FALSE;
        [self setFrameAndWineFrame:nonFullscreenFrame];
        [self windowDidResize:nil];
        if (pendingOrderOut)
            [self doOrderOut];
    }

    - (void) windowDidFailToEnterFullScreen:(NSWindow*)window
    {
        enteringFullScreen = FALSE;
        enteredFullScreenTime = 0;
        if (pendingOrderOut)
            [self doOrderOut];
    }

    - (void) windowDidFailToExitFullScreen:(NSWindow*)window
    {
        exitingFullScreen = FALSE;
        [self windowDidResize:nil];
        if (pendingOrderOut)
            [self doOrderOut];
    }

    - (void)windowDidMiniaturize:(NSNotification *)notification
    {
        macdrv_event* event;

        if (fullscreen && [self isOnActiveSpace])
            [[WineApplicationController sharedController] updateFullscreenWindows];

        [self checkWineDisplayLink];

        event = macdrv_create_event(WINDOW_DID_MINIMIZE, self);
        [queue postEvent:event];
        macdrv_release_event(event);
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

    - (void)windowDidResize:(NSNotification *)notification skipSizeMove:(BOOL)skipSizeMove
    {
        NSRect frame = self.wine_fractionalFrame;

        if ([self inLiveResize])
        {
            if (NSMinX(frame) != NSMinX(frameAtResizeStart))
                resizingFromLeft = TRUE;
            if (NSMaxY(frame) != NSMaxY(frameAtResizeStart))
                resizingFromTop = TRUE;
        }

        if (ignore_windowResize || exitingFullScreen) return;

        if ([self preventResizing])
        {
            NSRect contentRect = [self contentRectForFrameRect:frame];
            [self setContentMinSize:contentRect.size];
            [self setContentMaxSize:contentRect.size];
        }

        [self postWindowFrameChanged:frame
                          fullscreen:([self styleMask] & NSWindowStyleMaskFullScreen) != 0
                            resizing:[self inLiveResize]
                        skipSizeMove:skipSizeMove];

        [[[self contentView] inputContext] invalidateCharacterCoordinates];
        [self updateFullscreen];
    }

    - (void)windowDidResize:(NSNotification *)notification
    {
        [self windowDidResize:notification skipSizeMove:FALSE];
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
        nonFullscreenFrame = self.wine_fractionalFrame;
    }

    - (void) windowWillExitFullScreen:(NSNotification*)notification
    {
        exitingFullScreen = TRUE;
        [self postWindowFrameChanged:nonFullscreenFrame fullscreen:FALSE resizing:FALSE skipSizeMove:FALSE];
    }

    - (void)windowWillMiniaturize:(NSNotification *)notification
    {
        [self becameIneligibleParentOrChild];
        [self grabDockIconSnapshotFromWindow:nil force:NO];
    }

    - (NSSize) windowWillResize:(NSWindow*)sender toSize:(NSSize)frameSize
    {
        if ([self inLiveResize])
        {
            if (maximized)
                return self.wine_fractionalFrame.size;

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
            query->resize_size.rect = cgrect_win_from_mac(NSRectToCGRect(rect));
            query->resize_size.from_left = resizingFromLeft;
            query->resize_size.from_top = resizingFromTop;

            if ([self.queue query:query timeout:0.1])
            {
                rect = NSRectFromCGRect(cgrect_mac_from_win(query->resize_size.rect));
                rect = [self frameRectForContentRect:rect];
                frameSize = rect.size;
            }

            macdrv_release_query(query);
        }

        return frameSize;
    }

    - (void) windowWillStartLiveResize:(NSNotification *)notification
    {
        [self endWindowDragging];

        if (maximized)
        {
            macdrv_event* event;
            NSRect frame = [self contentRectForFrameRect:self.frame];

            [[WineApplicationController sharedController] flipRect:&frame];

            event = macdrv_create_event(WINDOW_RESTORE_REQUESTED, self);
            event->window_restore_requested.keep_frame = TRUE;
            event->window_restore_requested.frame = cgrect_win_from_mac(NSRectToCGRect(frame));
            [queue postEvent:event];
            macdrv_release_event(event);
        }
        else
            [self sendResizeStartQuery];

        frameAtResizeStart = [self frame];
        resizingFromLeft = resizingFromTop = FALSE;
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

    - (void) pasteboardChangedOwner:(NSPasteboard*)sender
    {
        macdrv_event* event = macdrv_create_event(LOST_PASTEBOARD_OWNERSHIP, self);
        [queue postEvent:event];
        macdrv_release_event(event);
    }


    /*
     * ---------- NSDraggingDestination methods ----------
     */
    - (NSDragOperation) draggingEntered:(id <NSDraggingInfo>)sender
    {
        macdrv_query* query = macdrv_create_query();
        NSPasteboard* pb = [sender draggingPasteboard];

        query->type = QUERY_DRAG_DROP_ENTER;
        query->window = (macdrv_window)[self retain];
        query->drag_drop.pasteboard = (CFTypeRef)[pb retain];

        [self.queue query:query timeout:0.1];
        macdrv_release_query(query);

        return [self draggingUpdated:sender];
    }

    - (void) draggingExited:(id <NSDraggingInfo>)sender
    {
        // This isn't really a query.  We don't need any response.  However, it
        // has to be processed in a similar manner as the other drag-and-drop
        // queries in order to maintain the proper order of operations.
        macdrv_query* query = macdrv_create_query();
        query->type = QUERY_DRAG_DROP_LEAVE;
        query->window = (macdrv_window)[self retain];

        [self.queue query:query timeout:0.1];
        macdrv_release_query(query);
    }

    - (NSDragOperation) draggingUpdated:(id <NSDraggingInfo>)sender
    {
        NSDragOperation ret;
        NSPoint pt = [[self contentView] convertPoint:[sender draggingLocation] fromView:nil];
        CGPoint cgpt = cgpoint_win_from_mac(NSPointToCGPoint(pt));

        macdrv_query* query = macdrv_create_query();
        query->type = QUERY_DRAG_DROP_DRAG;
        query->window = (macdrv_window)[self retain];
        query->drag_drop.x = floor(cgpt.x);
        query->drag_drop.y = floor(cgpt.y);
        query->drag_drop.ops = [sender draggingSourceOperationMask];

        [self.queue query:query timeout:3];
        ret = query->status ? query->drag_drop.ops : NSDragOperationNone;
        macdrv_release_query(query);

        return ret;
    }

    - (BOOL) performDragOperation:(id <NSDraggingInfo>)sender
    {
        BOOL ret;
        NSPoint pt = [[self contentView] convertPoint:[sender draggingLocation] fromView:nil];
        CGPoint cgpt = cgpoint_win_from_mac(NSPointToCGPoint(pt));

        macdrv_query* query = macdrv_create_query();
        query->type = QUERY_DRAG_DROP_DROP;
        query->window = (macdrv_window)[self retain];
        query->drag_drop.x = floor(cgpt.x);
        query->drag_drop.y = floor(cgpt.y);
        query->drag_drop.ops = [sender draggingSourceOperationMask];

        [self.queue query:query timeout:3 * 60 flags:WineQueryProcessEvents];
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
                                           windowFrame:NSRectFromCGRect(cgrect_mac_from_win(frame))
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
@autoreleasepool
{
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        window.closing = TRUE;
        [window doOrderOut];
        [window close];
    });
    [window.queue discardEventsMatchingMask:-1 forWindow:window];
    [window release];
}
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
@autoreleasepool
{
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
}
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
        [window setFrameFromWine:NSRectFromCGRect(cgrect_mac_from_win(*new_frame))];
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

        frame = [window contentRectForFrameRect:[window wine_fractionalFrame]];
        [[WineApplicationController sharedController] flipRect:&frame];
        *out_frame = cgrect_win_from_mac(NSRectToCGRect(frame));
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
 *              macdrv_window_set_color_image
 *
 * Push a window surface color pixel update in a specified rect (in non-client
 * area coordinates).
 */
void macdrv_window_set_color_image(macdrv_window w, CGImageRef image, CGRect rect, CGRect dirty)
{
@autoreleasepool
{
    WineWindow* window = (WineWindow*)w;

    CGImageRetain(image);

    OnMainThreadAsync(^{
        WineContentView *view = [window contentView];

        [view setColorImage:image];
        [view setSurfaceRect:cgrect_mac_from_win(rect)];
        [view setNeedsDisplayInRect:NSRectFromCGRect(cgrect_mac_from_win(dirty))];

        CGImageRelease(image);
    });
}
}


/***********************************************************************
 *              macdrv_window_set_shape_image
 */
void macdrv_window_set_shape_image(macdrv_window w, CGImageRef image)
{
@autoreleasepool
{
    WineWindow* window = (WineWindow*)w;

    CGImageRetain(image);

    OnMainThreadAsync(^{
        WineContentView *view = [window contentView];

        [view setShapeImage:image];
        [view setNeedsDisplay:true];
        [window checkTransparency];

        CGImageRelease(image);
    });
}
}

/***********************************************************************
 *              macdrv_set_window_shape
 *
 * Sets the shape of a Cocoa window from an array of rectangles.  If
 * rects is NULL, resets the window's shape to its frame.
 */
void macdrv_set_window_shape(macdrv_window w, const CGRect *rects, int count)
{
@autoreleasepool
{
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        if (!rects || !count)
        {
            [window setShape:NULL];
            [window checkEmptyShaped];
        }
        else
        {
            CGMutablePathRef path;
            unsigned int i;

            path = CGPathCreateMutable();
            for (i = 0; i < count; i++)
                CGPathAddRect(path, NULL, cgrect_mac_from_win(rects[i]));
            [window setShape:path];
            CGPathRelease(path);
        }
    });
}
}

/***********************************************************************
 *              macdrv_set_window_alpha
 */
void macdrv_set_window_alpha(macdrv_window w, CGFloat alpha)
{
@autoreleasepool
{
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        [window setAlphaValue:alpha];
    });
}
}

/***********************************************************************
 *              macdrv_window_use_per_pixel_alpha
 */
void macdrv_window_use_per_pixel_alpha(macdrv_window w, int use_per_pixel_alpha)
{
@autoreleasepool
{
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        window.usePerPixelAlpha = use_per_pixel_alpha;
        [window checkTransparency];
    });
}
}

/***********************************************************************
 *              macdrv_set_window_mask
 */
void macdrv_set_window_mask(macdrv_window w, CGRect rect)
{
@autoreleasepool
{
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        [window setMask:rect];
    });
}
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
        [window setWineMinSize:NSSizeFromCGSize(cgsize_mac_from_win(min_size)) maxSize:NSSizeFromCGSize(cgsize_mac_from_win(max_size))];
    });
}

/***********************************************************************
 *              macdrv_create_view
 *
 * Creates and returns a view with the specified frame rect.  The
 * caller is responsible for calling macdrv_dispose_view() on the view
 * when it is done with it.
 */
macdrv_view macdrv_create_view(CGRect rect)
{
@autoreleasepool
{
    __block WineContentView* view;

    if (CGRectIsNull(rect)) rect = CGRectZero;

    OnMainThread(^{
        NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];

        view = [[WineContentView alloc] initWithFrame:NSRectFromCGRect(cgrect_mac_from_win(rect))];
        [view setAutoresizingMask:NSViewNotSizable];
        [view setHidden:YES];
        [view setWantsBestResolutionOpenGLSurface:retina_on];
        [nc addObserver:view
               selector:@selector(updateGLContexts)
                   name:NSViewGlobalFrameDidChangeNotification
                 object:view];
        [nc addObserver:view
               selector:@selector(updateGLContexts)
                   name:NSApplicationDidChangeScreenParametersNotification
                 object:NSApp];
    });

    return (macdrv_view)view;
}
}

/***********************************************************************
 *              macdrv_dispose_view
 *
 * Destroys a view previously returned by macdrv_create_view.
 */
void macdrv_dispose_view(macdrv_view v)
{
@autoreleasepool
{
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
        [window updateForGLSubviews];
    });
}
}

/***********************************************************************
 *              macdrv_set_view_frame
 */
void macdrv_set_view_frame(macdrv_view v, CGRect rect)
{
@autoreleasepool
{
    WineContentView* view = (WineContentView*)v;

    if (CGRectIsNull(rect)) rect = CGRectZero;

    OnMainThreadAsync(^{
        NSRect newFrame = NSRectFromCGRect(cgrect_mac_from_win(rect));
        NSRect oldFrame = [view frame];

        if (!NSEqualRects(oldFrame, newFrame))
        {
            [[view superview] setNeedsDisplayInRect:oldFrame];
            if (NSEqualPoints(oldFrame.origin, newFrame.origin))
                [view setFrameSize:newFrame.size];
            else if (NSEqualSizes(oldFrame.size, newFrame.size))
                [view setFrameOrigin:newFrame.origin];
            else
                [view setFrame:newFrame];
            [view setNeedsDisplay:YES];
            [view wine_setBackingSize:(int[2]){ 0 }];
            [(WineWindow*)[view window] updateForGLSubviews];
        }
    });
}
}

/***********************************************************************
 *              macdrv_set_view_superview
 *
 * Move a view to a new superview and position it relative to its
 * siblings.  If p is non-NULL, the view is ordered behind it.
 * Otherwise, the view is ordered above n.  If s is NULL, use the
 * content view of w as the new superview.
 */
void macdrv_set_view_superview(macdrv_view v, macdrv_view s, macdrv_window w, macdrv_view p, macdrv_view n)
{
@autoreleasepool
{
    OnMainThreadAsync(^{
        WineContentView* view = (WineContentView*)v;
        WineContentView* superview = (WineContentView*)s;
        WineWindow* window = (WineWindow*)w;
        WineContentView* prev = (WineContentView*)p;
        WineContentView* next = (WineContentView*)n;

        if (!superview)
            superview = [window contentView];

        if (superview == [view superview])
        {
            NSArray* subviews = [superview subviews];
            NSUInteger index = [subviews indexOfObjectIdenticalTo:view];
            if (!prev && !next && index == [subviews count] - 1)
                return;
            if (prev && index + 1 < [subviews count] && subviews[index + 1] == prev)
                return;
            if (!prev && next && index > 0 && subviews[index - 1] == next)
                return;
        }

        WineWindow* oldWindow = (WineWindow*)[view window];
        WineWindow* newWindow = (WineWindow*)[superview window];

        if (prev)
            [superview addSubview:view positioned:NSWindowBelow relativeTo:prev];
        else
            [superview addSubview:view positioned:NSWindowAbove relativeTo:next];

        if (oldWindow != newWindow)
        {
            [oldWindow updateForGLSubviews];
            [newWindow updateForGLSubviews];
        }
    });
}
}

/***********************************************************************
 *              macdrv_set_view_hidden
 */
void macdrv_set_view_hidden(macdrv_view v, int hidden)
{
@autoreleasepool
{
    WineContentView* view = (WineContentView*)v;

    OnMainThreadAsync(^{
        [view setHidden:hidden];
        [(WineWindow*)view.window updateForGLSubviews];
    });
}
}

/***********************************************************************
 *              macdrv_add_view_opengl_context
 *
 * Add an OpenGL context to the list being tracked for each view.
 */
void macdrv_add_view_opengl_context(macdrv_view v, macdrv_opengl_context c)
{
@autoreleasepool
{
    WineContentView* view = (WineContentView*)v;
    WineOpenGLContext *context = (WineOpenGLContext*)c;

    OnMainThread(^{
        [view addGLContext:context];
    });
}
}

/***********************************************************************
 *              macdrv_remove_view_opengl_context
 *
 * Add an OpenGL context to the list being tracked for each view.
 */
void macdrv_remove_view_opengl_context(macdrv_view v, macdrv_opengl_context c)
{
@autoreleasepool
{
    WineContentView* view = (WineContentView*)v;
    WineOpenGLContext *context = (WineOpenGLContext*)c;

    OnMainThreadAsync(^{
        [view removeGLContext:context];
    });
}
}

macdrv_metal_device macdrv_create_metal_device(void)
{
@autoreleasepool
{
    return (macdrv_metal_device)MTLCreateSystemDefaultDevice();
}
}

void macdrv_release_metal_device(macdrv_metal_device d)
{
@autoreleasepool
{
    [(id<MTLDevice>)d release];
}
}

macdrv_metal_view macdrv_view_create_metal_view(macdrv_view v, macdrv_metal_device d)
{
    id<MTLDevice> device = (id<MTLDevice>)d;
    WineContentView* view = (WineContentView*)v;
    __block WineMetalView *metalView;

    OnMainThread(^{
        metalView = [view newMetalViewWithDevice:device];
    });

    return (macdrv_metal_view)metalView;
}

macdrv_metal_layer macdrv_view_get_metal_layer(macdrv_metal_view v)
{
    WineMetalView* view = (WineMetalView*)v;
    __block CAMetalLayer* layer;

    OnMainThread(^{
        layer = (CAMetalLayer*)view.layer;
    });

    return (macdrv_metal_layer)layer;
}

void macdrv_view_release_metal_view(macdrv_metal_view v)
{
    WineMetalView* view = (WineMetalView*)v;
    OnMainThread(^{
        [view removeFromSuperview];
        [view release];
    });
}

int macdrv_get_view_backing_size(macdrv_view v, int backing_size[2])
{
    WineContentView* view = (WineContentView*)v;

    if (![view isKindOfClass:[WineContentView class]])
        return FALSE;

    [view wine_getBackingSize:backing_size];
    return TRUE;
}

void macdrv_set_view_backing_size(macdrv_view v, const int backing_size[2])
{
    WineContentView* view = (WineContentView*)v;

    if ([view isKindOfClass:[WineContentView class]])
        [view wine_setBackingSize:backing_size];
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
 *              macdrv_send_keydown_to_input_source
 *
 * Sends a key down event to the active window's inputContext so that it can be
 * processed by input sources (AKA IMEs). This is only called when there is an
 * active non-keyboard input source.
 */
void macdrv_send_keydown_to_input_source(unsigned int flags, int repeat, int keyc, void* himc, int* done)
{
    OnMainThreadAsync(^{
        BOOL ret;
        macdrv_event* event;
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

            window.himc = himc;
            fix_device_modifiers_by_generic(&localFlags);

            // An NSEvent created with +keyEventWithType:... is internally marked
            // as synthetic and doesn't get sent through input methods.  But one
            // created from a CGEvent doesn't have that problem.
            c = CGEventCreateKeyboardEvent(NULL, keyc, true);
            CGEventSetFlags(c, localFlags);
            CGEventSetIntegerValueField(c, kCGKeyboardEventAutorepeat, repeat);
            event = [NSEvent eventWithCGEvent:c];
            CFRelease(c);

            window.commandDone = FALSE;
            ret = [[[window contentView] inputContext] handleEvent:event] && !window.commandDone;
        }
        else
            ret = FALSE;

        event = macdrv_create_event(SENT_TEXT_INPUT, window);
        event->sent_text_input.handled = ret;
        event->sent_text_input.done = done;
        [[window queue] postEvent:event];
        macdrv_release_event(event);
    });
}

void macdrv_clear_ime_text(void)
{
    OnMainThreadAsync(^{
        WineWindow* window = (WineWindow*)[NSApp keyWindow];
        if (![window isKindOfClass:[WineWindow class]])
        {
            window = (WineWindow*)[NSApp mainWindow];
            if (![window isKindOfClass:[WineWindow class]])
                window = [[WineApplicationController sharedController] frontWineWindow];
        }
        if (window)
            [[window contentView] clearMarkedText];
    });
}
