/*
 * Created by VisualGDB. Based on hello-jni example.
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.FancyTech.FancyUSBService;

import android.app.Activity;
import android.content.Intent;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.os.Bundle;
 
public class FancyUSBService extends Activity
{

	static String TAG = "FancyUSBService";
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        /* Create a Button and set its content.
         * the text is retrieved by calling a native
         * function.
         */
        final Button  button = new Button(this);
		button.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				button.setText(TAG);
			}
		});
        button.setText(TAG);
        setContentView(button);
        
        Intent intent = new Intent("com.sample.com.FancyTech.FancyUSBService.MY_ACTION");
        intent.setPackage("com.FancyTech.FancyUSBService");
        //Intent intent = new Intent(t, MyIntentService.class);
        startService(intent);
        
        Log.i(TAG, "startService(intent)"); 
        
        this.moveTaskToBack(true);
    }

    /* A native method that is implemented by the
     * 'FancyUSBService' native library, which is packaged
     * with this application.
     */
    //public native String  stringFromJNI();

    /* this is used to load the 'FancyUSBService' library on application
     * startup. The library has already been unpacked into
     * /data/data/com.FancyTech.FancyUSBService/lib/FancyUSBService.so at
     * installation time by the package manager.
     */
    static {
        System.loadLibrary("FancyUSBService");
    }
}
