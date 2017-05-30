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
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
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
        runOnUiThread( new Runnable() { public void run() { progress_dialog.dismiss(); }});

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
}
