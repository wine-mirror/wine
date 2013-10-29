/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * Wine GStreamer integration
 * Copyright 2010 Aric Stewart, CodeWeavers
 *
 * gthread.c: solaris thread system implementation
 * Copyright 1998-2001 Sebastian Wilhelmi; University of Karlsruhe
 * Copyright 2001 Hans Breuer
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

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#include "config.h"

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include <glib.h>

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gstreamer);

static gchar *
g_win32_error_message (gint error)
{
  gchar *retval;
  WCHAR *msg = NULL;
  int nchars;

  FormatMessageW (FORMAT_MESSAGE_ALLOCATE_BUFFER
          |FORMAT_MESSAGE_IGNORE_INSERTS
          |FORMAT_MESSAGE_FROM_SYSTEM,
          NULL, error, 0,
          (LPWSTR) &msg, 0, NULL);
  if (msg != NULL)
    {
      nchars = WideCharToMultiByte(CP_UTF8, 0, msg, -1, NULL, 0, NULL, NULL);

      if (nchars > 2 && msg[nchars-1] == '\n' && msg[nchars-2] == '\r')
          msg[nchars-2] = '\0';

      retval = g_utf16_to_utf8 (msg, -1, NULL, NULL, NULL);

      LocalFree (msg);
    }
  else
    retval = g_strdup ("");

  return retval;
}

static gint g_thread_priority_map [G_THREAD_PRIORITY_URGENT + 1] = {
    THREAD_PRIORITY_BELOW_NORMAL,
    THREAD_PRIORITY_NORMAL,
    THREAD_PRIORITY_ABOVE_NORMAL,
    THREAD_PRIORITY_HIGHEST
};

static DWORD g_thread_self_tls;

/* A "forward" declaration of this structure */
static GThreadFunctions g_thread_functions_for_glib_use_default;

typedef struct _GThreadData GThreadData;
struct _GThreadData
{
  GThreadFunc func;
  gpointer data;
  HANDLE thread;
  gboolean joinable;
};

static GMutex *
g_mutex_new_posix_impl (void)
{
  GMutex *result = (GMutex *) g_new (pthread_mutex_t, 1);
  pthread_mutex_init ((pthread_mutex_t *) result, NULL);
  return result;
}

static void
g_mutex_free_posix_impl (GMutex * mutex)
{
  pthread_mutex_destroy ((pthread_mutex_t *) mutex);
  g_free (mutex);
}

/* NOTE: the functions g_mutex_lock and g_mutex_unlock may not use
   functions from gmem.c and gmessages.c; */

/* pthread_mutex_lock, pthread_mutex_unlock can be taken directly, as
   signature and semantic are right, but without error check then!!!!,
   we might want to change this therefore. */

static gboolean
g_mutex_trylock_posix_impl (GMutex * mutex)
{
  int result;

  result = pthread_mutex_trylock ((pthread_mutex_t *) mutex);

  if (result == EBUSY)
    return FALSE;

  if (result) ERR("pthread_mutex_trylock %x\n",result);
  return TRUE;
}

static GCond *
g_cond_new_posix_impl (void)
{
  GCond *result = (GCond *) g_new (pthread_cond_t, 1);
  pthread_cond_init ((pthread_cond_t *) result, NULL);
  return result;
}

/* pthread_cond_signal, pthread_cond_broadcast and pthread_cond_wait
   can be taken directly, as signature and semantic are right, but
   without error check then!!!!, we might want to change this
   therefore. */

#define G_NSEC_PER_SEC 1000000000

static gboolean
g_cond_timed_wait_posix_impl (GCond * cond,
                              GMutex * entered_mutex,
                              GTimeVal * abs_time)
{
  int result;
  struct timespec end_time;
  gboolean timed_out;

  g_return_val_if_fail (cond != NULL, FALSE);
  g_return_val_if_fail (entered_mutex != NULL, FALSE);

  if (!abs_time)
    {
      result = pthread_cond_wait ((pthread_cond_t *)cond,
                                  (pthread_mutex_t *) entered_mutex);
      timed_out = FALSE;
    }
  else
    {
      end_time.tv_sec = abs_time->tv_sec;
      end_time.tv_nsec = abs_time->tv_usec * (G_NSEC_PER_SEC / G_USEC_PER_SEC);

      g_return_val_if_fail (end_time.tv_nsec < G_NSEC_PER_SEC, TRUE);

      result = pthread_cond_timedwait ((pthread_cond_t *) cond,
                                       (pthread_mutex_t *) entered_mutex,
                                       &end_time);
      timed_out = (result == ETIMEDOUT);
    }

  if (!timed_out)
    if (result) ERR("pthread_cond_timedwait %x\n",result);

  return !timed_out;
}

static void
g_cond_free_posix_impl (GCond * cond)
{
  pthread_cond_destroy ((pthread_cond_t *) cond);
  g_free (cond);
}

static GPrivate *
g_private_new_posix_impl (GDestroyNotify destructor)
{
  GPrivate *result = (GPrivate *) g_new (pthread_key_t, 1);
  pthread_key_create ((pthread_key_t *) result, destructor);
  return result;
}

/* NOTE: the functions g_private_get and g_private_set may not use
   functions from gmem.c and gmessages.c */

static void
g_private_set_posix_impl (GPrivate * private_key, gpointer value)
{
  if (!private_key)
    return;
  pthread_setspecific (*(pthread_key_t *) private_key, value);
}

static gpointer
g_private_get_posix_impl (GPrivate * private_key)
{
  if (!private_key)
    return NULL;
  return pthread_getspecific (*(pthread_key_t *) private_key);
}

static void
g_thread_set_priority_win32_impl (gpointer thread, GThreadPriority priority)
{
  GThreadData *target = *(GThreadData **)thread;

  g_return_if_fail ((int)priority >= G_THREAD_PRIORITY_LOW);
  g_return_if_fail ((int)priority <= G_THREAD_PRIORITY_URGENT);

  SetThreadPriority (target->thread, g_thread_priority_map [priority]);
}

static void
g_thread_self_win32_impl (gpointer thread)
{
  GThreadData *self = TlsGetValue (g_thread_self_tls);

  if (!self)
    {
      /* This should only happen for the main thread! */
      HANDLE handle = GetCurrentThread ();
      HANDLE process = GetCurrentProcess ();
      self = g_new (GThreadData, 1);
      DuplicateHandle (process, handle, process, &self->thread, 0, FALSE,
                        DUPLICATE_SAME_ACCESS);
      TlsSetValue (g_thread_self_tls, self);
      self->func = NULL;
      self->data = NULL;
      self->joinable = FALSE;
    }

  *(GThreadData **)thread = self;
}

static void
g_thread_exit_win32_impl (void)
{
  GThreadData *self = TlsGetValue (g_thread_self_tls);

  if (self)
    {
      if (!self->joinable)
      {
        CloseHandle (self->thread);
        g_free (self);
      }
      TlsSetValue (g_thread_self_tls, NULL);
    }

  ExitThread (0);
}

static guint __stdcall
g_thread_proxy (gpointer data)
{
  GThreadData *self = (GThreadData*) data;

  TlsSetValue (g_thread_self_tls, self);

  self->func (self->data);

  g_thread_exit_win32_impl ();

  g_assert_not_reached ();

  return 0;
}

static void
g_thread_create_win32_impl (GThreadFunc func,
                            gpointer data,
                            gulong stack_size,
                            gboolean joinable,
                            gboolean bound,
                            GThreadPriority priority,
                            gpointer thread,
                            GError **error)
{
  guint ignore;
  GThreadData *retval;

  g_return_if_fail (func);
  g_return_if_fail ((int)priority >= G_THREAD_PRIORITY_LOW);
  g_return_if_fail ((int)priority <= G_THREAD_PRIORITY_URGENT);

  retval = g_new(GThreadData, 1);
  retval->func = func;
  retval->data = data;

  retval->joinable = joinable;

  retval->thread = (HANDLE) CreateThread (NULL, stack_size, g_thread_proxy,
                                          retval, 0, &ignore);

  if (retval->thread == NULL)
    {
      gchar *win_error = g_win32_error_message (GetLastError ());
      g_set_error (error, G_THREAD_ERROR, G_THREAD_ERROR_AGAIN,
                   "Error creating thread: %s", win_error);
      g_free (retval);
      g_free (win_error);
      return;
    }

  *(GThreadData **)thread = retval;

  g_thread_set_priority_win32_impl (thread, priority);
}

static void
g_thread_yield_win32_impl (void)
{
  Sleep(0);
}

static void
g_thread_join_win32_impl (gpointer thread)
{
  GThreadData *target = *(GThreadData **)thread;

  g_return_if_fail (target->joinable);

  WaitForSingleObject (target->thread, INFINITE);

  CloseHandle (target->thread);
  g_free (target);
}

static GThreadFunctions g_thread_functions_for_glib_use_default =
{
  /* Posix functions here for speed */
  g_mutex_new_posix_impl,
  (void (*)(GMutex *)) pthread_mutex_lock,
  g_mutex_trylock_posix_impl,
  (void (*)(GMutex *)) pthread_mutex_unlock,
  g_mutex_free_posix_impl,
  g_cond_new_posix_impl,
  (void (*)(GCond *)) pthread_cond_signal,
  (void (*)(GCond *)) pthread_cond_broadcast,
  (void (*)(GCond *, GMutex *)) pthread_cond_wait,
  g_cond_timed_wait_posix_impl,
  g_cond_free_posix_impl,
  g_private_new_posix_impl,
  g_private_get_posix_impl,
  g_private_set_posix_impl,
  /* win32 function required here */
  g_thread_create_win32_impl,       /* thread */
  g_thread_yield_win32_impl,
  g_thread_join_win32_impl,
  g_thread_exit_win32_impl,
  g_thread_set_priority_win32_impl,
  g_thread_self_win32_impl,
  NULL                             /* no equal function necessary */
};

void g_thread_impl_init (void)
{
  static gboolean beenhere = FALSE;

  if (beenhere)
    return;

  beenhere = TRUE;

  g_thread_self_tls = TlsAlloc ();
  g_thread_init(&g_thread_functions_for_glib_use_default);
}
