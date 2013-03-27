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

#import "cocoa_opengl.h"

#include "macdrv_cocoa.h"
#include "cocoa_event.h"


@interface WineOpenGLContext ()
@property (retain, nonatomic) NSView* latentView;
@end


@implementation WineOpenGLContext
@synthesize latentView, needsUpdate;

    - (void) dealloc
    {
        [latentView release];
        [super dealloc];
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
        static NSWindow* dummyWindow;
        static dispatch_once_t once;

        dispatch_once(&once, ^{
            OnMainThread(^{
                dummyWindow = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 100, 100)
                                                          styleMask:NSBorderlessWindowMask
                                                            backing:NSBackingStoreBuffered
                                                              defer:NO];
            });
        });

        [self setView:[dummyWindow contentView]];
        [self clearDrawable];
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
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineOpenGLContext *context;

    context = [[WineOpenGLContext alloc] initWithCGLContextObj:cglctx];

    [pool release];
    return (macdrv_opengl_context)context;
}

/***********************************************************************
 *              macdrv_dispose_opengl_context
 *
 * Destroys a Cocoa OpenGL context previously created by
 * macdrv_create_opengl_context();
 */
void macdrv_dispose_opengl_context(macdrv_opengl_context c)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineOpenGLContext *context = (WineOpenGLContext*)c;

    if ([context view])
        macdrv_remove_view_opengl_context((macdrv_view)[context view], c);
    if ([context latentView])
        macdrv_remove_view_opengl_context((macdrv_view)[context latentView], c);
    [context clearDrawableLeavingSurfaceOnScreen];
    [context release];

    [pool release];
}

/***********************************************************************
 *              macdrv_make_context_current
 */
void macdrv_make_context_current(macdrv_opengl_context c, macdrv_view v)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineOpenGLContext *context = (WineOpenGLContext*)c;
    NSView* view = (NSView*)v;

    if (context)
    {
        if ([context view])
            macdrv_remove_view_opengl_context((macdrv_view)[context view], c);
        if ([context latentView])
            macdrv_remove_view_opengl_context((macdrv_view)[context latentView], c);
        context.needsUpdate = FALSE;
        if (view)
        {
            __block BOOL windowHasDevice;

            macdrv_add_view_opengl_context(v, c);

            OnMainThread(^{
                windowHasDevice = [[view window] windowNumber] > 0;
            });
            if (windowHasDevice)
            {
                [context setView:view];
                [context setLatentView:nil];
            }
            else
                [context setLatentView:view];

            [context makeCurrentContext];
        }
        else
        {
            [WineOpenGLContext clearCurrentContext];
            [context clearDrawableLeavingSurfaceOnScreen];
        }
    }
    else
        [WineOpenGLContext clearCurrentContext];

    [pool release];
}

/***********************************************************************
 *              macdrv_update_opengl_context
 */
void macdrv_update_opengl_context(macdrv_opengl_context c)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineOpenGLContext *context = (WineOpenGLContext*)c;

    if (context.needsUpdate)
    {
        context.needsUpdate = FALSE;
        if (context.latentView)
        {
            [context setView:context.latentView];
            context.latentView = nil;
        }
        else
            [context update];
    }

    [pool release];
}

/***********************************************************************
 *              macdrv_flush_opengl_context
 *
 * Performs an implicit glFlush() and then swaps the back buffer to the
 * front (if the context is double-buffered).
 */
void macdrv_flush_opengl_context(macdrv_opengl_context c)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    WineOpenGLContext *context = (WineOpenGLContext*)c;

    macdrv_update_opengl_context(c);
    [context flushBuffer];

    [pool release];
}
