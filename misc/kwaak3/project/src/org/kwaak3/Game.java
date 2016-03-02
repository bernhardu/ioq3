/*
 * Kwaak3
 * Copyright (C) 2010 Roderick Colenbrander
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package org.kwaak3;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.util.Log;
import android.view.Window;
import android.view.WindowManager;

import org.libsdl.app.SDLActivity;
import android.view.*;

public class Game extends SDLActivity {
        public static boolean mEnableSound;
        public static boolean mEnableLightmaps;
        public static boolean mEnableBenchmark;
        public static boolean mEnableShowFPS;

	private void showError(String s)
	{
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setMessage(s);
		builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int id) {
				Game.this.finish();
			}
		});
		AlertDialog dialog = builder.create();
		dialog.show();
	}

	private boolean checkGameFiles()
	{
		File quake3_dir = new File("/sdcard/quake3");
		if(quake3_dir.exists() == false)
		{
			showError("Unable to locate /sdcard/quake3.");
			return false;
		}

		File baseq3_dir = new File("/sdcard/quake3/baseq3");
		if(baseq3_dir.exists() == false)
		{
			showError("Unable to locate /sdcard/quake3/baseq3.");
			return false;
		}

		/* Check if pak0.pk3-pak8.pk3 are around */
		for(int i=0; i<8; i++)
		{
			String pak_filename = "pak" + i + ".pk3";
			File quake3_pak_file = new File("/sdcard/quake3/baseq3/" + pak_filename);
			if(quake3_pak_file.exists() == false)
			{
				showError("Unable to locate /sdcard/quake3/baseq3/"+pak_filename);
				return false;
			}
		}
		return true;
	}

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		/* We like to be fullscreen */
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
		requestWindowFeature(Window.FEATURE_NO_TITLE);

                Bundle extras = getIntent().getExtras();
                if(extras != null) {
                    mEnableSound = extras.getBoolean("sound");
                    mEnableLightmaps = extras.getBoolean("lightmaps");
                    mEnableBenchmark = extras.getBoolean("benchmark");
                    mEnableShowFPS = extras.getBoolean("showfps");
                }
                /* Don't initialize the game engine when the game files aren't around */
                if (!checkGameFiles()) {
			setContentView(R.layout.main);
                } else {
                    super.onCreate(savedInstanceState);
                }
        }

        @Override
        protected String[] getLibraries() {
            return new String[] {
                "kwaakjni"
            };
        }

        @Override
        protected String[] getArguments() {
            List<String> list = new ArrayList<String>();

            list.add("+set");
            list.add("r_mode");
            list.add("11"); /*"Mode 11: 856x480 (wide)"*/

            if (!mEnableSound) {
                list.add("+set");
                list.add("s_initsound");
                list.add(mEnableSound ? "0" : "1");
            }

            list.add("+set");
            list.add("r_vertexlight");
            list.add(mEnableLightmaps ? "0" : "1");

            list.add("+set");
            list.add("cg_drawfps");
            list.add(mEnableShowFPS ? "1" : "0");

            if(mEnableBenchmark) {
                list.add("+demo");
                list.add("four");
                list.add("+timedemo");
                list.add("1");
            }

            list.add("+set");
            list.add("fs_homepath");
            list.add(android.os.Environment.getExternalStorageDirectory().getPath() + "/quake3");

            list.add("+set");
            list.add("fs_basepath");
            list.add("/data/data/org.kwaak3/lib");

            /* prefer compiled objects over interpreted */
            list.add("+set");
            list.add("vm_cgame");
            list.add("0");

            list.add("+set");
            list.add("vm_game");
            list.add("0");

            list.add("+set");
            list.add("vm_ui");
            list.add("0");

            Log.d("Quake_JAVA", "starting with arguments: " + list);

            String[] ret = new String[ list.size() ];
            list.toArray(ret);
            return ret;
        }

	@Override
	protected void onDestroy() {
		super.onDestroy();
	}

	@Override
	protected void onPause() {
		//Log.d("Quake_JAVA", "onPause");
		super.onPause();
	}

	@Override
	protected void onResume() {
		/* Resume doesn't always seem to work well. On my Milestone it works
		 * but not on the G1. The native code seems to be running but perhaps
		 * we need to issue a 'vid_restart'.
		 */
		//Log.d("Quake_JAVA", "onResume");
		super.onResume();
	}
}
