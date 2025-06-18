/*
 * WineActivity class
 *
 * Copyright 2013-2017 Alexandre Julliard
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

package org.winehq.wine;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.PointerIcon;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import android.view.ViewGroup;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

public class WineActivity extends Activity
{
    private native String wine_init( String[] cmdline, String[] env );
    public native void wine_desktop_changed( int width, int height );
    public native void wine_config_changed( int dpi );
    public native void wine_surface_changed( int hwnd, Surface surface, boolean opengl );
    public native boolean wine_motion_event( int hwnd, int action, int x, int y, int state, int vscroll );
    public native boolean wine_keyboard_event( int hwnd, int action, int keycode, int state );

    private final String LOGTAG = "wine";
    private ProgressDialog progress_dialog;

    protected WineWindow desktop_window;
    protected WineWindow message_window;
    private PointerIcon current_cursor;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate( savedInstanceState );

        requestWindowFeature( android.view.Window.FEATURE_NO_TITLE );

        new Thread( new Runnable() { public void run() { loadWine( null ); }} ).start();
    }

    @TargetApi(21)
    @SuppressWarnings("deprecation")
    private String[] get_supported_abis()
    {
        if (Build.VERSION.SDK_INT >= 21) return Build.SUPPORTED_ABIS;
        return new String[]{ Build.CPU_ABI };
    }

    private String get_wine_abi()
    {
        for (String abi : get_supported_abis())
        {
            File server = new File( getFilesDir(), abi + "/bin/wineserver" );
            if (server.canExecute()) return abi;
        }
        Log.e( LOGTAG, "could not find a supported ABI" );
        return null;
    }

    private String get_so_dir( String abi )
    {
        if (abi.equals( "x86" )) return "/i386-unix";
        if (abi.equals( "x86_64" )) return "/x86_64-unix";
        if (abi.equals( "armeabi-v7a" )) return "/arm-unix";
        if (abi.equals( "arm64-v8a" )) return "/aarch64-unix";
        return "";
    }

    private void loadWine( String cmdline )
    {
        copyAssetFiles();

        String wine_abi = get_wine_abi();
        File bindir = new File( getFilesDir(), wine_abi + "/bin" );
        File libdir = new File( getFilesDir(), wine_abi + "/lib" );
        File dlldir = new File( libdir, "wine" );
        File prefix = new File( getFilesDir(), "prefix" );
        File loader = new File( bindir, "wine" );
        String locale = Locale.getDefault().getLanguage() + "_" +
            Locale.getDefault().getCountry() + ".UTF-8";

        HashMap<String,String> env = new HashMap<String,String>();
        env.put( "WINELOADER", loader.toString() );
        env.put( "WINEPREFIX", prefix.toString() );
        env.put( "WINEDLLPATH", dlldir.toString() );
        env.put( "LD_LIBRARY_PATH", libdir.toString() + ":" + getApplicationInfo().nativeLibraryDir );
        env.put( "LC_ALL", locale );
        env.put( "LANG", locale );
        env.put( "PATH", bindir.toString() + ":" + System.getenv( "PATH" ));

        if (cmdline == null)
        {
            if (new File( prefix, "drive_c/winestart.cmd" ).exists()) cmdline = "c:\\winestart.cmd";
            else cmdline = "c:\\windows\\system32\\wineconsole.exe";
        }

        String winedebug = readFileString( new File( prefix, "winedebug" ));
        if (winedebug == null) winedebug = readFileString( new File( getFilesDir(), "winedebug" ));
        if (winedebug != null)
        {
            File log = new File( getFilesDir(), "log" );
            env.put( "WINEDEBUG", winedebug );
            env.put( "WINEDEBUGLOG", log.toString() );
            Log.i( LOGTAG, "logging to " + log.toString() );
            log.delete();
        }

        createProgressDialog( 0, "Setting up the Windows environment..." );

        System.load( dlldir.toString() + get_so_dir(wine_abi) + "/ntdll.so" );
        prefix.mkdirs();

        runWine( cmdline, env );
    }

    private final void runWine( String cmdline, HashMap<String,String> environ )
    {
        String[] env = new String[environ.size() * 2];
        int j = 0;
        for (Map.Entry<String,String> entry : environ.entrySet())
        {
            env[j++] = entry.getKey();
            env[j++] = entry.getValue();
        }

        String[] cmd = { environ.get( "WINELOADER" ),
                         "c:\\windows\\system32\\explorer.exe",
                         "/desktop=shell,,android",
                         cmdline };

        String err = wine_init( cmd, env );
        Log.e( LOGTAG, err );
    }

    private void createProgressDialog( final int max, final String message )
    {
        runOnUiThread( new Runnable() { public void run() {
            if (progress_dialog != null) progress_dialog.dismiss();
            progress_dialog = new ProgressDialog( WineActivity.this );
            progress_dialog.setProgressStyle( max > 0 ? ProgressDialog.STYLE_HORIZONTAL
                                              : ProgressDialog.STYLE_SPINNER );
            progress_dialog.setTitle( "Wine" );
            progress_dialog.setMessage( message );
            progress_dialog.setCancelable( false );
            progress_dialog.setMax( max );
            progress_dialog.show();
        }});

    }

    private final boolean isFileWanted( String name )
    {
        if (name.equals( "files.sum" )) return true;
        if (name.startsWith( "share/" )) return true;
        for (String abi : get_supported_abis())
        {
            if (name.startsWith( abi + "/system/" )) return false;
            if (name.startsWith( abi + "/" )) return true;
        }
        if (name.startsWith( "x86/" )) return true;
        return false;
    }

    private final boolean isFileExecutable( String name )
    {
        return !name.equals( "files.sum" ) && !name.startsWith( "share/" );
    }

    private final HashMap<String,String> readMapFromInputStream( InputStream in )
    {
        HashMap<String,String> map = new HashMap<String,String>();
        String str;

        try
        {
            BufferedReader reader = new BufferedReader( new InputStreamReader( in, "UTF-8" ));
            while ((str = reader.readLine()) != null)
            {
                String entry[] = str.split( "\\s+", 2 );
                if (entry.length == 2 && isFileWanted( entry[1] )) map.put( entry[1], entry[0] );
            }
        }
        catch( IOException e ) { }
        return map;
    }

    private final HashMap<String,String> readMapFromDiskFile( String file )
    {
        try
        {
            return readMapFromInputStream( new FileInputStream( new File( getFilesDir(), file )));
        }
        catch( IOException e ) { return new HashMap<String,String>(); }
    }

    private final HashMap<String,String> readMapFromAssetFile( String file )
    {
        try
        {
            return readMapFromInputStream( getAssets().open( file ) );
        }
        catch( IOException e ) { return new HashMap<String,String>(); }
    }

    private final String readFileString( File file )
    {
        try
        {
            FileInputStream in = new FileInputStream( file );
            BufferedReader reader = new BufferedReader( new InputStreamReader( in, "UTF-8" ));
            return reader.readLine();
        }
        catch( IOException e ) { return null; }
    }

    private final void copyAssetFile( String src )
    {
        File dest = new File( getFilesDir(), src );
        try
        {
            Log.i( LOGTAG, "extracting " + dest );
            dest.getParentFile().mkdirs();
            dest.delete();
            if (dest.createNewFile())
            {
                InputStream in = getAssets().open( src );
                FileOutputStream out = new FileOutputStream( dest );
                int read;
                byte[] buffer = new byte[65536];

                while ((read = in.read( buffer )) > 0) out.write( buffer, 0, read );
                out.close();
                if (isFileExecutable( src )) dest.setExecutable( true, true );
            }
            else
                Log.i( LOGTAG, "Failed to create file "  + dest );
        }
        catch( IOException e )
        {
            Log.i( LOGTAG, "Failed to copy asset file to " + dest );
            dest.delete();
        }
    }

    private final void deleteAssetFile( String src )
    {
        File dest = new File( getFilesDir(), src );
        Log.i( LOGTAG, "deleting " + dest );
        dest.delete();
    }

    private final void copyAssetFiles()
    {
        String new_sum = readMapFromAssetFile( "sums.sum" ).get( "files.sum" );
        if (new_sum == null) return;  // no assets

        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences( this );
        String old_sum = prefs.getString( "files.sum", "" );
        if (old_sum.equals( new_sum )) return;  // no change
        prefs.edit().putString( "files.sum", new_sum ).apply();

        HashMap<String,String> existing_files = readMapFromDiskFile( "files.sum" );
        HashMap<String,String> new_files = readMapFromAssetFile( "files.sum" );
        ArrayList<String> copy_files = new ArrayList<String>();
        copy_files.add( "files.sum" );

        for (Map.Entry<String, String> entry : new_files.entrySet())
        {
            String name = entry.getKey();
            if (!entry.getValue().equals( existing_files.remove( name ))) copy_files.add( name );
        }

        createProgressDialog( copy_files.size(), "Extracting files..." );

        for (String name : existing_files.keySet()) deleteAssetFile( name );

        for (String name : copy_files)
        {
            copyAssetFile( name );
            runOnUiThread( new Runnable() { public void run() {
                progress_dialog.incrementProgressBy( 1 ); }});
        }
    }

    //
    // Generic Wine window class
    //

    private HashMap<Integer,WineWindow> win_map = new HashMap<Integer,WineWindow>();

    protected class WineWindow
    {
        static protected final int HWND_MESSAGE = 0xfffffffd;
        static protected final int SWP_NOZORDER = 0x04;
        static protected final int WS_VISIBLE = 0x10000000;

        protected int hwnd;
        protected int owner;
        protected int style;
        protected float scale;
        protected boolean visible;
        protected Rect visible_rect;
        protected Rect client_rect;
        protected WineWindow parent;
        protected ArrayList<WineWindow> children;
        protected Surface window_surface;
        protected Surface client_surface;
        protected SurfaceTexture window_surftex;
        protected SurfaceTexture client_surftex;
        protected WineWindowGroup window_group;
        protected WineWindowGroup client_group;

        public WineWindow( int w, WineWindow parent, float scale )
        {
            Log.i( LOGTAG, String.format( "create hwnd %08x", w ));
            hwnd = w;
            owner = 0;
            style = 0;
            visible = false;
            visible_rect = client_rect = new Rect( 0, 0, 0, 0 );
            this.parent = parent;
            this.scale = scale;
            children = new ArrayList<WineWindow>();
            win_map.put( w, this );
            if (parent != null) parent.children.add( this );
        }

        public void destroy()
        {
            Log.i( LOGTAG, String.format( "destroy hwnd %08x", hwnd ));
            visible = false;
            win_map.remove( this );
            if (parent != null) parent.children.remove( this );
            destroy_window_groups();
        }

        public WineWindowGroup create_window_groups()
        {
            if (client_group != null) return client_group;
            window_group = new WineWindowGroup( this );
            client_group = new WineWindowGroup( this );
            window_group.addView( client_group );
            client_group.set_layout( client_rect.left - visible_rect.left,
                                     client_rect.top - visible_rect.top,
                                     client_rect.right - visible_rect.left,
                                     client_rect.bottom - visible_rect.top );
            if (parent != null)
            {
                parent.create_window_groups();
                if (visible) add_view_to_parent();
                window_group.set_layout( visible_rect.left, visible_rect.top,
                                         visible_rect.right, visible_rect.bottom );
            }
            return client_group;
        }

        public void destroy_window_groups()
        {
            if (window_group != null)
            {
                if (parent != null && parent.client_group != null) remove_view_from_parent();
                window_group.destroy_view();
            }
            if (client_group != null) client_group.destroy_view();
            window_group = null;
            client_group = null;
        }

        public View create_whole_view()
        {
            if (window_group == null) create_window_groups();
            window_group.create_view( false ).layout( 0, 0,
                                                      Math.round( (visible_rect.right - visible_rect.left) * scale ),
                                                      Math.round( (visible_rect.bottom - visible_rect.top) * scale ));
            window_group.set_scale( scale );
            return window_group;
        }

        public void create_client_view()
        {
            if (client_group == null) create_window_groups();
            Log.i( LOGTAG, String.format( "creating client view %08x %s", hwnd, client_rect ));
            client_group.create_view( true ).layout( 0, 0, client_rect.right - client_rect.left,
                                                     client_rect.bottom - client_rect.top );
        }

        protected void add_view_to_parent()
        {
            int pos = parent.client_group.getChildCount() - 1;

            // the content view is always last
            if (pos >= 0 && parent.client_group.getChildAt( pos ) == parent.client_group.get_content_view()) pos--;

            for (int i = 0; i < parent.children.size() && pos >= 0; i++)
            {
                WineWindow child = parent.children.get( i );
                if (child == this) break;
                if (!child.visible) continue;
                if (child == ((WineWindowGroup)parent.client_group.getChildAt( pos )).get_window()) pos--;
            }
            parent.client_group.addView( window_group, pos + 1 );
        }

        protected void remove_view_from_parent()
        {
            parent.client_group.removeView( window_group );
        }

        protected void set_zorder( WineWindow prev )
        {
            int pos = -1;

            parent.children.remove( this );
            if (prev != null) pos = parent.children.indexOf( prev );
            parent.children.add( pos + 1, this );
        }

        protected void sync_views_zorder()
        {
            int i, j;

            for (i = parent.children.size() - 1, j = 0; i >= 0; i--)
            {
                WineWindow child = parent.children.get( i );
                if (!child.visible) continue;
                View child_view = parent.client_group.getChildAt( j );
                if (child_view == parent.client_group.get_content_view()) continue;
                if (child != ((WineWindowGroup)child_view).get_window()) break;
                j++;
            }
            while (i >= 0)
            {
                WineWindow child = parent.children.get( i-- );
                if (child.visible) child.window_group.bringToFront();
            }
        }

        public void pos_changed( int flags, int insert_after, int owner, int style,
                                 Rect window_rect, Rect client_rect, Rect visible_rect )
        {
            boolean was_visible = visible;
            this.visible_rect = visible_rect;
            this.client_rect = client_rect;
            this.style = style;
            this.owner = owner;
            visible = (style & WS_VISIBLE) != 0;

            Log.i( LOGTAG, String.format( "pos changed hwnd %08x after %08x owner %08x style %08x win %s client %s visible %s flags %08x",
                                          hwnd, insert_after, owner, style, window_rect, client_rect, visible_rect, flags ));

            if ((flags & SWP_NOZORDER) == 0 && parent != null) set_zorder( get_window( insert_after ));

            if (window_group != null)
            {
                window_group.set_layout( visible_rect.left, visible_rect.top,
                                         visible_rect.right, visible_rect.bottom );
                if (parent != null)
                {
                    if (!was_visible && (style & WS_VISIBLE) != 0) add_view_to_parent();
                    else if (was_visible && (style & WS_VISIBLE) == 0) remove_view_from_parent();
                    else if (visible && (flags & SWP_NOZORDER) == 0) sync_views_zorder();
                }
            }

            if (client_group != null)
                client_group.set_layout( client_rect.left - visible_rect.left,
                                         client_rect.top - visible_rect.top,
                                         client_rect.right - visible_rect.left,
                                         client_rect.bottom  - visible_rect.top );
        }

        public void set_parent( WineWindow new_parent, float scale )
        {
            Log.i( LOGTAG, String.format( "set parent hwnd %08x parent %08x -> %08x",
                                          hwnd, parent.hwnd, new_parent.hwnd ));
            this.scale = scale;
            if (window_group != null)
            {
                if (visible) remove_view_from_parent();
                new_parent.create_window_groups();
                window_group.set_layout( visible_rect.left, visible_rect.top,
                                         visible_rect.right, visible_rect.bottom );
            }
            parent.children.remove( this );
            parent = new_parent;
            parent.children.add( this );
            if (visible && window_group != null) add_view_to_parent();
        }

        public int get_hwnd()
        {
            return hwnd;
        }

        private void update_surface( boolean is_client )
        {
            if (is_client)
            {
                Log.i( LOGTAG, String.format( "set client surface hwnd %08x %s", hwnd, client_surface ));
                if (client_surface != null) wine_surface_changed( hwnd, client_surface, true );
            }
            else
            {
                Log.i( LOGTAG, String.format( "set window surface hwnd %08x %s", hwnd, window_surface ));
                if (window_surface != null) wine_surface_changed( hwnd, window_surface, false );
            }
        }

        public void set_surface( SurfaceTexture surftex, boolean is_client )
        {
            if (is_client)
            {
                if (surftex == null) client_surface = null;
                else if (surftex != client_surftex)
                {
                    client_surftex = surftex;
                    client_surface = new Surface( surftex );
                }
            }
            else
            {
                if (surftex == null) window_surface = null;
                else if (surftex != window_surftex)
                {
                    window_surftex = surftex;
                    window_surface = new Surface( surftex );
                }
            }
            update_surface( is_client );
        }

        public void get_event_pos( MotionEvent event, int[] pos )
        {
            pos[0] = Math.round( event.getX() * scale + window_group.getLeft() );
            pos[1] = Math.round( event.getY() * scale + window_group.getTop() );
        }
    }

    //
    // Window group for a Wine window, optionally containing a content view
    //

    protected class WineWindowGroup extends ViewGroup
    {
        private WineView content_view;
        private WineWindow win;

        WineWindowGroup( WineWindow win )
        {
            super( WineActivity.this );
            this.win = win;
            setVisibility( View.VISIBLE );
        }

        /* wrapper for layout() making sure that the view is not empty */
        public void set_layout( int left, int top, int right, int bottom )
        {
            left   *= win.scale;
            top    *= win.scale;
            right  *= win.scale;
            bottom *= win.scale;
            if (right <= left + 1) right = left + 2;
            if (bottom <= top + 1) bottom = top + 2;
            layout( left, top, right, bottom );
        }

        @Override
        protected void onLayout( boolean changed, int left, int top, int right, int bottom )
        {
            if (content_view != null) content_view.layout( 0, 0, right - left, bottom - top );
        }

        public void set_scale( float scale )
        {
            if (content_view == null) return;
            content_view.setPivotX( 0 );
            content_view.setPivotY( 0 );
            content_view.setScaleX( scale );
            content_view.setScaleY( scale );
        }

        public WineView create_view( boolean is_client )
        {
            if (content_view != null) return content_view;
            content_view = new WineView( WineActivity.this, win, is_client );
            addView( content_view );
            if (!is_client)
            {
                content_view.setFocusable( true );
                content_view.setFocusableInTouchMode( true );
            }
            return content_view;
        }

        public void destroy_view()
        {
            if (content_view == null) return;
            removeView( content_view );
            content_view = null;
        }

        public WineView get_content_view()
        {
            return content_view;
        }

        public WineWindow get_window()
        {
            return win;
        }
    }

    // View used for all Wine windows, backed by a TextureView

    protected class WineView extends TextureView implements TextureView.SurfaceTextureListener
    {
        private WineWindow window;
        private boolean is_client;

        public WineView( Context c, WineWindow win, boolean client )
        {
            super( c );
            window = win;
            is_client = client;
            setSurfaceTextureListener( this );
            setVisibility( VISIBLE );
            setOpaque( false );
            setFocusable( true );
            setFocusableInTouchMode( true );
        }

        public WineWindow get_window()
        {
            return window;
        }

        @Override
        public void onSurfaceTextureAvailable( SurfaceTexture surftex, int width, int height )
        {
            Log.i( LOGTAG, String.format( "onSurfaceTextureAvailable win %08x %dx%d %s",
                                          window.hwnd, width, height, is_client ? "client" : "whole" ));
            window.set_surface( surftex, is_client );
        }

        @Override
        public void onSurfaceTextureSizeChanged( SurfaceTexture surftex, int width, int height )
        {
            Log.i( LOGTAG, String.format( "onSurfaceTextureSizeChanged win %08x %dx%d %s",
                                          window.hwnd, width, height, is_client ? "client" : "whole" ));
            window.set_surface( surftex, is_client);
        }

        @Override
        public boolean onSurfaceTextureDestroyed( SurfaceTexture surftex )
        {
            Log.i( LOGTAG, String.format( "onSurfaceTextureDestroyed win %08x %s",
                                          window.hwnd, is_client ? "client" : "whole" ));
            window.set_surface( null, is_client );
            return false;  // hold on to the texture since the app may still be using it
        }

        @Override
        public void onSurfaceTextureUpdated(SurfaceTexture surftex)
        {
        }

        @TargetApi(24)
        public PointerIcon onResolvePointerIcon( MotionEvent event, int index )
        {
            return current_cursor;
        }

        @Override
        public boolean onGenericMotionEvent( MotionEvent event )
        {
            if (is_client) return false;  // let the whole window handle it
            if (window.parent != null && window.parent != desktop_window) return false;  // let the parent handle it

            if ((event.getSource() & InputDevice.SOURCE_CLASS_POINTER) != 0)
            {
                int[] pos = new int[2];
                window.get_event_pos( event, pos );
                Log.i( LOGTAG, String.format( "view motion event win %08x action %d pos %d,%d buttons %04x view %d,%d",
                                              window.hwnd, event.getAction(), pos[0], pos[1],
                                              event.getButtonState(), getLeft(), getTop() ));
                return wine_motion_event( window.hwnd, event.getAction(), pos[0], pos[1],
                                          event.getButtonState(), (int)event.getAxisValue(MotionEvent.AXIS_VSCROLL)  );
            }
            return super.onGenericMotionEvent(event);
        }

        @Override
        public boolean onTouchEvent( MotionEvent event )
        {
            if (is_client) return false;  // let the whole window handle it
            if (window.parent != null && window.parent != desktop_window) return false;  // let the parent handle it

            int[] pos = new int[2];
            window.get_event_pos( event, pos );
            Log.i( LOGTAG, String.format( "view touch event win %08x action %d pos %d,%d buttons %04x view %d,%d",
                                          window.hwnd, event.getAction(), pos[0], pos[1],
                                          event.getButtonState(), getLeft(), getTop() ));
            return wine_motion_event( window.hwnd, event.getAction(), pos[0], pos[1],
                                      event.getButtonState(), 0 );
        }

        @Override
        public boolean dispatchKeyEvent( KeyEvent event )
        {
            Log.i( LOGTAG, String.format( "view key event win %08x action %d keycode %d (%s)",
                                          window.hwnd, event.getAction(), event.getKeyCode(),
                                          KeyEvent.keyCodeToString( event.getKeyCode() )));;
            boolean ret = wine_keyboard_event( window.hwnd, event.getAction(), event.getKeyCode(),
                                               event.getMetaState() );
            if (!ret) ret = super.dispatchKeyEvent(event);
            return ret;
        }
    }

    // The top-level desktop view group

    protected class TopView extends ViewGroup
    {
        public TopView( Context context, int hwnd )
        {
            super( context );
            desktop_window = new WineWindow( hwnd, null, 1.0f );
            addView( desktop_window.create_whole_view() );
            desktop_window.client_group.bringToFront();

            message_window = new WineWindow( WineWindow.HWND_MESSAGE, null, 1.0f );
            message_window.create_window_groups();
        }

        @Override
        protected void onSizeChanged( int width, int height, int old_width, int old_height )
        {
            Log.i( LOGTAG, String.format( "desktop size %dx%d", width, height ));
            wine_desktop_changed( width, height );
        }

        @Override
        protected void onLayout( boolean changed, int left, int top, int right, int bottom )
        {
            // nothing to do
        }
    }

    protected WineWindow get_window( int hwnd )
    {
        return win_map.get( hwnd );
    }

    // Entry points for the device driver

    public void create_desktop_window( int hwnd )
    {
        Log.i( LOGTAG, String.format( "create desktop view %08x", hwnd ));
        setContentView( new TopView( this, hwnd ));
        progress_dialog.dismiss();
        wine_config_changed( getResources().getConfiguration().densityDpi );
    }

    public void create_window( int hwnd, boolean opengl, int parent, float scale, int pid )
    {
        WineWindow win = get_window( hwnd );
        if (win == null)
        {
            win = new WineWindow( hwnd, get_window( parent ), scale );
            win.create_window_groups();
            if (win.parent == desktop_window) win.create_whole_view();
        }
        if (opengl) win.create_client_view();
    }

    public void destroy_window( int hwnd )
    {
        WineWindow win = get_window( hwnd );
        if (win != null) win.destroy();
    }

    public void set_window_parent( int hwnd, int parent, float scale, int pid )
    {
        WineWindow win = get_window( hwnd );
        if (win == null) return;
        win.set_parent( get_window( parent ), scale );
        if (win.parent == desktop_window) win.create_whole_view();
    }

    @TargetApi(24)
    public void set_cursor( int id, int width, int height, int hotspotx, int hotspoty, int bits[] )
    {
        Log.i( LOGTAG, String.format( "set_cursor id %d size %dx%d hotspot %dx%d", id, width, height, hotspotx, hotspoty ));
        if (bits != null)
        {
            Bitmap bitmap = Bitmap.createBitmap( bits, width, height, Bitmap.Config.ARGB_8888 );
            current_cursor = PointerIcon.create( bitmap, hotspotx, hotspoty );
        }
        else current_cursor = PointerIcon.getSystemIcon( this, id );
    }

    public void window_pos_changed( int hwnd, int flags, int insert_after, int owner, int style,
                                    Rect window_rect, Rect client_rect, Rect visible_rect )
    {
        WineWindow win = get_window( hwnd );
        if (win != null)
            win.pos_changed( flags, insert_after, owner, style, window_rect, client_rect, visible_rect );
    }

    public void createDesktopWindow( final int hwnd )
    {
        runOnUiThread( new Runnable() { public void run() { create_desktop_window( hwnd ); }} );
    }

    public void createWindow( final int hwnd, final boolean opengl, final int parent, final float scale, final int pid )
    {
        runOnUiThread( new Runnable() { public void run() { create_window( hwnd, opengl, parent, scale, pid ); }} );
    }

    public void destroyWindow( final int hwnd )
    {
        runOnUiThread( new Runnable() { public void run() { destroy_window( hwnd ); }} );
    }

    public void setParent( final int hwnd, final int parent, final float scale, final int pid )
    {
        runOnUiThread( new Runnable() { public void run() { set_window_parent( hwnd, parent, scale, pid ); }} );
    }

    public void setCursor( final int id, final int width, final int height,
                           final int hotspotx, final int hotspoty, final int bits[] )
    {
        if (Build.VERSION.SDK_INT < 24) return;
        runOnUiThread( new Runnable() { public void run() { set_cursor( id, width, height, hotspotx, hotspoty, bits ); }} );
    }

    public void windowPosChanged( final int hwnd, final int flags, final int insert_after,
                                  final int owner, final int style,
                                  final int window_left, final int window_top,
                                  final int window_right, final int window_bottom,
                                  final int client_left, final int client_top,
                                  final int client_right, final int client_bottom,
                                  final int visible_left, final int visible_top,
                                  final int visible_right, final int visible_bottom )
    {
        final Rect window_rect = new Rect( window_left, window_top, window_right, window_bottom );
        final Rect client_rect = new Rect( client_left, client_top, client_right, client_bottom );
        final Rect visible_rect = new Rect( visible_left, visible_top, visible_right, visible_bottom );
        runOnUiThread( new Runnable() {
            public void run() { window_pos_changed( hwnd, flags, insert_after, owner, style,
                                                    window_rect, client_rect, visible_rect ); }} );
    }
}
