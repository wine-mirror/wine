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

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
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
    public native void wine_surface_changed( int hwnd, Surface surface );
    public native boolean wine_motion_event( int hwnd, int action, int x, int y, int state, int vscroll );
    public native boolean wine_keyboard_event( int hwnd, int action, int keycode, int state );

    private final String LOGTAG = "wine";
    private ProgressDialog progress_dialog;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate( savedInstanceState );

        requestWindowFeature( android.view.Window.FEATURE_NO_TITLE );

        new Thread( new Runnable() { public void run() { loadWine( null ); }} ).start();
    }

    private void loadWine( String cmdline )
    {
        File bindir = new File( getFilesDir(), Build.CPU_ABI + "/bin" );
        File libdir = new File( getFilesDir(), Build.CPU_ABI + "/lib" );
        File prefix = new File( getFilesDir(), "prefix" );
        File loader = new File( bindir, "wine" );
        String locale = Locale.getDefault().getLanguage() + "_" +
            Locale.getDefault().getCountry() + ".UTF-8";

        copyAssetFiles();

        HashMap<String,String> env = new HashMap<String,String>();
        env.put( "WINELOADER", loader.toString() );
        env.put( "WINEPREFIX", prefix.toString() );
        env.put( "LD_LIBRARY_PATH", libdir.toString() );
        env.put( "LC_ALL", locale );
        env.put( "LANG", locale );

        if (cmdline == null)
        {
            if (new File( prefix, "drive_c/winestart.cmd" ).exists()) cmdline = "c:\\winestart.cmd";
            else cmdline = "wineconsole.exe";
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

        System.load( libdir.toString() + "/libwine.so" );
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
                         "explorer.exe",
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
        if (name.startsWith( Build.CPU_ABI + "/system/" )) return false;
        if (name.startsWith( Build.CPU_ABI + "/" )) return true;
        if (name.startsWith( "x86/" )) return true;
        return false;
    }

    private final boolean isFileExecutable( String name )
    {
        return name.startsWith( Build.CPU_ABI + "/" ) || name.startsWith( "x86/" );
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

    protected class WineWindow extends Object
    {
        static protected final int WS_VISIBLE = 0x10000000;

        protected int hwnd;
        protected int owner;
        protected int style;
        protected boolean visible;
        protected Rect visible_rect;
        protected Rect client_rect;
        protected WineWindow parent;
        protected WineView window_view;
        protected Surface window_surface;

        public WineWindow( int w, WineWindow parent )
        {
            Log.i( LOGTAG, String.format( "create hwnd %08x", w ));
            hwnd = w;
            owner = 0;
            style = 0;
            visible = false;
            visible_rect = client_rect = new Rect( 0, 0, 0, 0 );
            this.parent = parent;
            win_map.put( w, this );
            if (parent == null)
            {
                window_view = new WineView( WineActivity.this, this );
                window_view.layout( 0, 0, 1, 1 ); // make sure the surface gets created
            }
        }

        public void destroy()
        {
            Log.i( LOGTAG, String.format( "destroy hwnd %08x", hwnd ));
            if (visible && window_view != null) top_view.removeView( window_view );
            visible = false;
            window_view = null;
            win_map.remove( this );
        }

        public void pos_changed( int flags, int insert_after, int owner, int style,
                                 Rect window_rect, Rect client_rect, Rect visible_rect )
        {
            this.visible_rect = visible_rect;
            this.client_rect = client_rect;
            this.style = style;
            this.owner = owner;
            Log.i( LOGTAG, String.format( "pos changed hwnd %08x after %08x owner %08x style %08x win %s client %s visible %s flags %08x",
                                          hwnd, insert_after, owner, style, window_rect, client_rect, visible_rect, flags ));

            if (parent == null)
            {
                window_view.layout( visible_rect.left, visible_rect.top, visible_rect.right, visible_rect.bottom );
                if (!visible && (style & WS_VISIBLE) != 0) top_view.addView( window_view );
                else if (visible && (style & WS_VISIBLE) == 0) top_view.removeView( window_view );
            }
            visible = (style & WS_VISIBLE) != 0;
        }

        public void set_parent( WineWindow new_parent )
        {
            Log.i( LOGTAG, String.format( "set parent hwnd %08x parent %08x -> %08x",
                                          hwnd, parent == null ? 0 : parent.hwnd,
                                          new_parent == null ? 0 : new_parent.hwnd ));
            parent = new_parent;
            if (new_parent == null)
            {
                window_view = new WineView( WineActivity.this, this );
                window_view.layout( 0, 0, 1, 1 ); // make sure the surface gets created
            }
            else window_view = null;
        }

        public int get_hwnd()
        {
            return hwnd;
        }

        public void set_surface( SurfaceTexture surftex )
        {
            if (surftex == null) window_surface = null;
            else if (window_surface == null) window_surface = new Surface( surftex );
            Log.i( LOGTAG, String.format( "set window surface hwnd %08x %s", hwnd, window_surface ));
            wine_surface_changed( hwnd, window_surface );
        }

        public void get_event_pos( MotionEvent event, int[] pos )
        {
            pos[0] = Math.round( event.getX() + window_view.getLeft() );
            pos[1] = Math.round( event.getY() + window_view.getTop() );
        }
    }

    // View used for all Wine windows, backed by a TextureView

    protected class WineView extends TextureView implements TextureView.SurfaceTextureListener
    {
        private WineWindow window;

        public WineView( Context c, WineWindow win )
        {
            super( c );
            window = win;
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

        public void onSurfaceTextureAvailable( SurfaceTexture surftex, int width, int height )
        {
            Log.i( LOGTAG, String.format( "onSurfaceTextureAvailable win %08x %dx%d",
                                          window.hwnd, width, height ));
            window.set_surface( surftex );
        }

        public void onSurfaceTextureSizeChanged( SurfaceTexture surftex, int width, int height )
        {
            Log.i( LOGTAG, String.format( "onSurfaceTextureSizeChanged win %08x %dx%d",
                                          window.hwnd, width, height ));
            window.set_surface( surftex );
        }

        public boolean onSurfaceTextureDestroyed( SurfaceTexture surftex )
        {
            Log.i( LOGTAG, String.format( "onSurfaceTextureDestroyed win %08x", window.hwnd ));
            window.set_surface( null );
            return true;
        }

        public void onSurfaceTextureUpdated(SurfaceTexture surftex)
        {
        }

        public boolean onGenericMotionEvent( MotionEvent event )
        {
            if (window.parent != null) return false;  // let the parent handle it

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

        public boolean onTouchEvent( MotionEvent event )
        {
            if (window.parent != null) return false;  // let the parent handle it

            int[] pos = new int[2];
            window.get_event_pos( event, pos );
            Log.i( LOGTAG, String.format( "view touch event win %08x action %d pos %d,%d buttons %04x view %d,%d",
                                          window.hwnd, event.getAction(), pos[0], pos[1],
                                          event.getButtonState(), getLeft(), getTop() ));
            return wine_motion_event( window.hwnd, event.getAction(), pos[0], pos[1],
                                      event.getButtonState(), 0 );
        }

        public boolean dispatchKeyEvent( KeyEvent event )
        {
            Log.i( LOGTAG, String.format( "view key event win %08x action %d keycode %d (%s)",
                                          window.hwnd, event.getAction(), event.getKeyCode(),
                                          event.keyCodeToString( event.getKeyCode() )));;
            boolean ret = wine_keyboard_event( window.hwnd, event.getAction(), event.getKeyCode(),
                                               event.getMetaState() );
            if (!ret) ret = super.dispatchKeyEvent(event);
            return ret;
        }
    }

    // The top-level desktop view group

    protected class TopView extends ViewGroup
    {
        private int desktop_hwnd;

        public TopView( Context context, int hwnd )
        {
            super( context );
            desktop_hwnd = hwnd;
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

    protected TopView top_view;

    protected WineWindow get_window( int hwnd )
    {
        return win_map.get( hwnd );
    }

    // Entry points for the device driver

    public void create_desktop_window( int hwnd )
    {
        Log.i( LOGTAG, String.format( "create desktop view %08x", hwnd ));
        top_view = new TopView( this, hwnd );
        setContentView( top_view );
        progress_dialog.dismiss();
    }

    public void create_window( int hwnd, int parent, int pid )
    {
        WineWindow win = get_window( hwnd );
        if (win == null) win = new WineWindow( hwnd, get_window( parent ));
    }

    public void destroy_window( int hwnd )
    {
        WineWindow win = get_window( hwnd );
        if (win != null) win.destroy();
    }

    public void set_window_parent( int hwnd, int parent, int pid )
    {
        WineWindow win = get_window( hwnd );
        if (win == null) return;
        win.set_parent( get_window( parent ));
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

    public void createWindow( final int hwnd, final int parent, final int pid )
    {
        runOnUiThread( new Runnable() { public void run() { create_window( hwnd, parent, pid ); }} );
    }

    public void destroyWindow( final int hwnd )
    {
        runOnUiThread( new Runnable() { public void run() { destroy_window( hwnd ); }} );
    }

    public void setParent( final int hwnd, final int parent, final int pid )
    {
        runOnUiThread( new Runnable() { public void run() { set_window_parent( hwnd, parent, pid ); }} );
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
