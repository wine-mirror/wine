/*
 * MACDRV Cocoa OpenGL code
 *
 * Copyright 2012, 2013 Ken Thomases for CodeWeavers Inc.
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

#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#import "cocoa_opengl.h"

#include "macdrv_cocoa.h"
#include "cocoa_app.h"
#include "cocoa_event.h"

#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"


@interface WineOpenGLContext ()
@property (retain, nonatomic) NSView* latentView;

    + (NSView*) dummyView;
    - (void) wine_updateBackingSize:(const CGSize*)size;

@end


@implementation WineOpenGLContext
@synthesize latentView, needsUpdate, needsReattach;

    - (void) dealloc
    {
        [[self view] release];
        [latentView release];
        [super dealloc];
    }

    + (NSView*) dummyView
    {
        static NSWindow* dummyWindow;
        static NSView* dummyWindowContentView;
        static dispatch_once_t once;

        dispatch_once(&once, ^{
            OnMainThread(^{
                dummyWindow = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 100, 100)
                                                          styleMask:NSWindowStyleMaskBorderless
                                                            backing:NSBackingStoreBuffered
                                                              defer:NO];
                dummyWindowContentView = dummyWindow.contentView;
            });
        });

        return dummyWindowContentView;
    }

    // Normally, we take care that disconnecting a context from a view doesn't
    // destroy that view's GL surface (see -clearDrawableLeavingSurfaceOnScreen).
    // However, if we're using a surface backing size and that size changes, we
    // need to destroy and recreate the surface or we get weird behavior.
    - (void) resetSurfaceIfBackingSizeChanged
    {
        int view_backing[2];
        if (macdrv_get_view_backing_size((macdrv_view)self.view, view_backing) &&
            (view_backing[0] != backing_size[0] || view_backing[1] != backing_size[1]))
        {
            view_backing[0] = backing_size[0];
            view_backing[1] = backing_size[1];
            macdrv_set_view_backing_size((macdrv_view)self.view, view_backing);

            NSView* save = self.view;
            if ([NSThread isMainThread])
            {
                [super clearDrawable];
                [super setView:save];
            }
            else OnMainThread(^{
                [super clearDrawable];
                [super setView:save];
            });
        }
    }

    /* This is a no-op if either dimension of the size is zero. */
    - (void) wine_updateBackingSize:(const CGSize*)size
    {
        GLint enabled;

        if (size)
        {
            if (size->width == 0 || size->height == 0)
                return;

            if (CGLIsEnabled(self.CGLContextObj, kCGLCESurfaceBackingSize, &enabled) != kCGLNoError)
                enabled = 0;

            if (!enabled || backing_size[0] != size->width || backing_size[1] != size->height)
            {
                backing_size[0] = size->width;
                backing_size[1] = size->height;
                CGLSetParameter(self.CGLContextObj, kCGLCPSurfaceBackingSize, backing_size);
            }

            if (!enabled)
                CGLEnable(self.CGLContextObj, kCGLCESurfaceBackingSize);

            [self resetSurfaceIfBackingSizeChanged];
        }
        else
        {
            backing_size[0] = 0;
            backing_size[1] = 0;

            if (CGLIsEnabled(self.CGLContextObj, kCGLCESurfaceBackingSize, &enabled) == kCGLNoError && enabled)
               CGLDisable(self.CGLContextObj, kCGLCESurfaceBackingSize);
        }
    }

    - (void) setView:(NSView*)newView
    {
        NSView* oldView = [self view];
        if ([NSThread isMainThread])
            [super setView:newView];
        else OnMainThread(^{
            [super setView:newView];
        });
        [newView retain];
        [oldView release];
    }

    - (void) clearDrawable
    {
        NSView* oldView = [self view];
        if ([NSThread isMainThread])
            [super clearDrawable];
        else OnMainThread(^{
            [super clearDrawable];
        });
        [oldView release];

        [self wine_updateBackingSize:NULL];
    }

    /* On at least some versions of Mac OS X, -[NSOpenGLContext clearDrawable] has the
       undesirable side effect of ordering the view's GL surface off-screen.  This isn't
       done when just changing the context's view to a different view (which I would
       think would be analogous, since the old view and surface end up without a
       context attached).  So, we finesse things by first setting the context's view to
       a different view (the content view of an off-screen window) and then letting the
       original implementation proceed. */
    - (void) clearDrawableLeavingSurfaceOnScreen
    {
        [self setView:[[self class] dummyView]];
        [self clearDrawable];
    }

    - (void) removeFromViews:(BOOL)removeViews
    {
        if ([self view])
        {
            macdrv_remove_view_opengl_context((macdrv_view)[self view], (macdrv_opengl_context)self);
            if (removeViews)
                [self clearDrawableLeavingSurfaceOnScreen];
        }
        if ([self latentView])
        {
            macdrv_remove_view_opengl_context((macdrv_view)[self latentView], (macdrv_opengl_context)self);
            if (removeViews)
                [self setLatentView:nil];
        }
        needsUpdate = FALSE;
        needsReattach = FALSE;
    }

@end


/***********************************************************************
 *              macdrv_create_opengl_context
 *
 * Returns a Cocoa OpenGL context created from a CoreGL context.  The
 * caller is responsible for calling macdrv_dispose_opengl_context()
 * when done with the context object.
 */
macdrv_opengl_context macdrv_create_opengl_context(void* cglctx)
{
@autoreleasepool
{
    WineOpenGLContext *context;

    context = [[WineOpenGLContext alloc] initWithCGLContextObj:cglctx];

    return (macdrv_opengl_context)context;
}
}

/***********************************************************************
 *              macdrv_dispose_opengl_context
 *
 * Destroys a Cocoa OpenGL context previously created by
 * macdrv_create_opengl_context();
 */
void macdrv_dispose_opengl_context(macdrv_opengl_context c)
{
@autoreleasepool
{
    WineOpenGLContext *context = (WineOpenGLContext*)c;

    [context removeFromViews:YES];
    [context release];
}
}

/***********************************************************************
 *              macdrv_make_context_current
 */
void macdrv_make_context_current(macdrv_opengl_context c, macdrv_view v, CGRect r)
{
@autoreleasepool
{
    WineOpenGLContext *context = (WineOpenGLContext*)c;
    NSView* view = (NSView*)v;

    if (context && view)
    {
        if (view == [context view] || view == [context latentView])
        {
            [context wine_updateBackingSize:&r.size];
            macdrv_update_opengl_context(c);
        }
        else
        {
            [context removeFromViews:NO];
            macdrv_add_view_opengl_context(v, c);

            if (context.needsUpdate)
            {
                context.needsUpdate = FALSE;
                context.needsReattach = FALSE;
                if (context.view)
                    [context setView:[[context class] dummyView]];
                [context wine_updateBackingSize:&r.size];
                [context setView:view];
                [context setLatentView:nil];
                [context resetSurfaceIfBackingSizeChanged];
            }
            else
            {
                if ([context view])
                    [context clearDrawableLeavingSurfaceOnScreen];
                [context wine_updateBackingSize:&r.size];
                [context setLatentView:view];
            }
        }

        [context makeCurrentContext];
    }
    else
    {
        WineOpenGLContext* currentContext = (WineOpenGLContext*)[WineOpenGLContext currentContext];

        if ([currentContext isKindOfClass:[WineOpenGLContext class]])
        {
            [WineOpenGLContext clearCurrentContext];
            if (currentContext != context)
                [currentContext removeFromViews:YES];
        }

        if (context)
            [context removeFromViews:YES];
    }
}
}

/***********************************************************************
 *              macdrv_update_opengl_context
 */
void macdrv_update_opengl_context(macdrv_opengl_context c)
{
@autoreleasepool
{
    WineOpenGLContext *context = (WineOpenGLContext*)c;

    if (context.needsUpdate)
    {
        BOOL reattach = context.needsReattach;
        context.needsUpdate = FALSE;
        context.needsReattach = FALSE;
        if (context.latentView)
        {
            [context setView:context.latentView];
            context.latentView = nil;

            [context resetSurfaceIfBackingSizeChanged];
        }
        else
        {
            if (reattach)
            {
                NSView* view = [[context.view retain] autorelease];
                [context clearDrawableLeavingSurfaceOnScreen];
                context.view = view;
            }
            else OnMainThread(^{
                [context update];
            });
            [context resetSurfaceIfBackingSizeChanged];
        }
    }
}
}

/***********************************************************************
 *              macdrv_flush_opengl_context
 *
 * Performs an implicit glFlush() and then swaps the back buffer to the
 * front (if the context is double-buffered).
 */
void macdrv_flush_opengl_context(macdrv_opengl_context c)
{
@autoreleasepool
{
    WineOpenGLContext *context = (WineOpenGLContext*)c;

    macdrv_update_opengl_context(c);
    [context flushBuffer];
}
}
