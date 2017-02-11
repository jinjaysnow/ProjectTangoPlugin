package com.projecttango.plugin;

import java.io.File;
import java.io.IOException;
import android.net.Uri;
import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.database.Cursor;
import android.content.Intent;
import android.widget.Toast;
import android.util.Log;
import android.content.ServiceConnection;
import android.os.Build;
import android.os.IBinder;
import android.content.ComponentName;

public class TangoInterface {
    
    /////NOTE: Functions and enums borrowed from TangoInitializationHelper.java in the C examples
    
    public static final int ARCH_ERROR = -2;
    public static final int ARCH_FALLBACK = -1;
    public static final int ARCH_DEFAULT = 0;
    public static final int ARCH_ARM64 = 1;
    public static final int ARCH_ARM32 = 2;
    public static final int ARCH_X86_64 = 3;
    public static final int ARCH_X86 = 4;

    File cwd;
    
    // Tango Service connection.
    ServiceConnection mTangoServiceConnection = new ServiceConnection()
    {
        public void onServiceConnected(ComponentName name, IBinder service)
        {
            // Synchronization around HelloMotionTrackingActivity object is to avoid
            // Tango disconnect in the middle of the connecting operation.
            //@NOTE: Do we need a reference to the activity class here instead?
            synchronized (TangoInterface.this)
            {
                //Call the native functions to complete starting the service
                OnJavaTangoServiceConnected(service);
            }
        }

        public void onServiceDisconnected(ComponentName name)
        {
            // Handle this if you need to gracefully shutdown/retry
            // in the event that Tango itself crashes/gets upgraded while running.
        }
    };

//Use a different activity code in order to distinguish between calls
	public static final int TANGO_INTENT_ACTIVITYCODE = 1129;
	public static final int TANGO_IMPORT_INTENT_ACTIVITYCODE = 1130;
	public static final int TANGO_EXPORT_INTENT_ACTIVITYCODE = 1131;

	public static final String EXTRA_KEY_PERMISSIONTYPE = "PERMISSIONTYPE";
	public static final String EXTRA_VALUE_ADF = "ADF_LOAD_SAVE_PERMISSION";
	private static final String EXTRA_KEY_SOURCEUUID = "SOURCE_UUID";
	private static final String EXTRA_KEY_SOURCEFILE = "SOURCE_FILE";
	private static final String EXTRA_KEY_DESTINATIONFILE = "DESTINATION_FILE";
	
	private static final String INTENT_CLASSPACKAGE = "com.google.tango";
	private static final String INTENT_IMPORTEXPORT_CLASSNAME = "com.google.atap.tango.RequestImportExportActivity";


	private static final String TAG = "TANGO";
	private boolean mIsTangoConnectionReady = false;

	private boolean IsTangoAreaLearningEnabled = false;
	private boolean HasAttemptedPreviousIntentCall = false;


    
	public TangoInterface(boolean IsTangoAreaLearningEnabled) {

		this.IsTangoAreaLearningEnabled = IsTangoAreaLearningEnabled;
	}

	public void pause() {
		Log.w("TangoLifecycleDebugging", "Activity stopped! Connection is no longer ready...");
		mIsTangoConnectionReady = false;
	}

	public void resume(Activity currentActivity) {
		if (!mIsTangoConnectionReady && !HasAttemptedPreviousIntentCall) 
		{
	
			//Avoid attempting the event call twice!
			HasAttemptedPreviousIntentCall = true;

			//@TODO: Introduce the permissions for TangoAreaLearningSave/Load, TangoADFImport and TangoADFExport.
			if (IsTangoAreaLearningEnabled) 
			{
				//Check to see if we've already got this permission
				if(!hasPermission(currentActivity, EXTRA_VALUE_ADF))
				{
			        //Log.w(TAG, "About to create a new Intent.");
					Intent ADF_LoadSave_Intent = new Intent();
					ADF_LoadSave_Intent.setAction("android.intent.action.REQUEST_TANGO_PERMISSION");
					ADF_LoadSave_Intent.putExtra(EXTRA_KEY_PERMISSIONTYPE, EXTRA_VALUE_ADF);
				
					Log.w("TangoLifecycleDebugging", "Created Intent for ADF!");
			
					currentActivity.startActivityForResult(ADF_LoadSave_Intent, TANGO_INTENT_ACTIVITYCODE);
					Log.w("TangoLifecycleDebugging", "Started ADF event activity for result!");
				}
				else
				{
			        Log.w(TAG, "Permissions were found for this permission type.");
				}
			}
			else
			{
				mIsTangoConnectionReady = true;
			}
		}
    }

    //Tango service binding
    public void RequestTangoService(final Context context)
    {
        //@TODO: Connect to the Tango Java service
        bindTangoService(context, mTangoServiceConnection);
    }

    //Tango service unbinding- To be called after the C library has disconnected from Tango.
    public void UnbindTangoService(final Context context)
    {
        context.unbindService(mTangoServiceConnection);
    }

    //Native call to function which will call BindAndCompleteTangoService for us
    public static native void OnJavaTangoServiceConnected(IBinder binder);

    public static native void OnImportRequest(int result);

    public static native void OnExportRequest(int result);


        public void requestImportPermissions(Activity currentActivity, String Filepath)
	{
		Log.w("TangoLifecycleDebugging", "TangoInterface.Java::requestImportPermissions: Starting function!");
		Log.w("TangoLifecycleDebugging", "Filepath String = " + Filepath);
		Intent exportIntent = new Intent();
		exportIntent.setClassName(INTENT_CLASSPACKAGE, INTENT_IMPORTEXPORT_CLASSNAME);
		exportIntent.putExtra(EXTRA_KEY_SOURCEFILE, Filepath);
		currentActivity.startActivityForResult(exportIntent, TANGO_IMPORT_INTENT_ACTIVITYCODE);

	}

	public void requestExportPermissions(Activity currentActivity, String UUID, String Filepath)
	{
		Log.w("TangoLifecycleDebugging", "TangoInterface.Java::requestExportPermissions: Starting function!");

		Log.w("TangoLifecycleDebugging", "UUID String = " + UUID);
		Log.w("TangoLifecycleDebugging", "Filepath String = " + Filepath);
		//Prepare the folder for export
		getAppSpaceADFFolder(Filepath);
		Intent exportIntent = new Intent();
		exportIntent.setClassName(INTENT_CLASSPACKAGE, INTENT_IMPORTEXPORT_CLASSNAME);
		exportIntent.putExtra(EXTRA_KEY_SOURCEUUID, UUID);
		exportIntent.putExtra(EXTRA_KEY_DESTINATIONFILE, Filepath);
		currentActivity.startActivityForResult(exportIntent, TANGO_EXPORT_INTENT_ACTIVITYCODE);

	}

	public boolean handleActivityResult(int requestCode, int resultCode, Intent data, Activity currentActivity) 
	{
		Log.w("TangoLifecycleDebugging", "Starting to handle Activity Result...");

		if (requestCode == TANGO_INTENT_ACTIVITYCODE) 
		{
		
			Log.w("TangoLifecycleDebugging", "Recieved Tango Intent Activity Code!");

			// Make sure the request was successful
			if (resultCode == Activity.RESULT_CANCELED) 
			{
				Log.w("TangoLifecycleDebugging", "Tango Intent Activity: Result is User Cancelled!");
				Toast.makeText(currentActivity, "This app requires Tango permissions!", Toast.LENGTH_LONG).show();
				return false;
			}

			Log.w("TangoLifecycleDebugging", "Tango Intent Activity; result was not cancelled, setting connection flag to 'Ready'!");
			mIsTangoConnectionReady = true;

			return true;
		}
		else if (requestCode == TANGO_IMPORT_INTENT_ACTIVITYCODE)
		{
			Log.w("TangoLifecycleDebugging", "Tango Import Intent Activity; result is being handled!");
			if (resultCode == -1) 
			{
				Log.w("TangoLifecycleDebugging", "Tango Import Intent Activity: Result is Success!");
				//Toast.makeText(currentActivity, "Tango Import Intent Activity: Result is Success, -1!", Toast.LENGTH_LONG).show();
                                OnImportRequest(-1);
				return true;
			}			
			if (resultCode == 0) 
			{
				Log.w("TangoLifecycleDebugging", "Tango Import Intent Activity: Result is Cancelled!");
				//Toast.makeText(currentActivity, "Tango Import Intent Activity: Result is Cancelled, 0!", Toast.LENGTH_LONG).show();
                                OnImportRequest(0);
				return false;
			}
			if (resultCode == 1) 
			{
				Log.w("TangoLifecycleDebugging", "Tango Import Intent Activity: Result is User Denied!");
				//Toast.makeText(currentActivity, "Tango Import Intent Activity: Result is Denied, 1!", Toast.LENGTH_LONG).show();
                                OnImportRequest(1);
				return false;
			}

			Toast.makeText(currentActivity, "Tango Import Intent Activity: Result unknown", Toast.LENGTH_LONG).show();
			Log.w("TangoLifecycleDebugging", "Tango Import Intent Activity: Result unknown!");		
			return true;
		}
		else if (requestCode == TANGO_EXPORT_INTENT_ACTIVITYCODE)
		{
			Log.w("TangoLifecycleDebugging", "Tango Export Intent Activity; result is being handled!");
			if (resultCode == -1) 
			{
				Log.w("TangoLifecycleDebugging", "Tango Export Intent Activity: Result is Success!");
				//Toast.makeText(currentActivity, "Tango Export Intent Activity: Result is Success, -1!", Toast.LENGTH_LONG).show();
                                OnExportRequest(-1);
				return true;
			}			
			if (resultCode == 0) 
			{
				Log.w("TangoLifecycleDebugging", "Tango Export Intent Activity: Result is Cancelled!");
				//Toast.makeText(currentActivity, "Tango Export Intent Activity: Result is Cancelled, 0!", Toast.LENGTH_LONG).show();
                                OnExportRequest(0);
                                return false;
			}
			if (resultCode == 1) 
			{
				Log.w("TangoLifecycleDebugging", "Tango Export Intent Activity: Result is User Denied!");
				//Toast.makeText(currentActivity, "Tango Export Intent Activity: Result is Denied, 1!", Toast.LENGTH_LONG).show();
                                OnExportRequest(1);
                                return false;
			}

			Toast.makeText(currentActivity, "Tango Export Intent Activity: Result unknown", Toast.LENGTH_LONG).show();
			Log.w("TangoLifecycleDebugging", "Tango Export Intent Activity: Result unknown!");		
			return true;
		}
		
		Log.w("TangoLifecycleDebugging", "Found a non-Tango activity result!");
		return false;
	}

	private void getAppSpaceADFFolder(String Filepath) 
	{
		Log.w("TangoLifecycleDebugging", "Tango Export: Preparing new folder.");
		File file = new File(Filepath);
		if (!file.exists()) {
		Log.w("TangoLifecycleDebugging", "Tango Export: File does not yet exist.");
			file.mkdirs();
		}
		Log.w("TangoLifecycleDebugging", "Tango Export: Completed file setup!");

		return;
	}

	public static boolean hasPermission(Context context, String permissionType){
	    
	    Uri uri = Uri.parse("content://com.google.atap.tango.PermissionStatusProvider/" +
	            permissionType);
	    Cursor cursor = context.getContentResolver().query(uri, null, null, null, null);
	    if (cursor == null) {
	        return false;
	    } else {
	        return true;
	    }
	}
    
    /**
     * Only for apps using the C API:
     * Initializes the underlying TangoService for native apps.
     *
     * @return returns false if the device doesn't have the Tango running as Android Service.
     *     Otherwise ture.
     */
    public static final boolean bindTangoService(final Context context,
                                                 ServiceConnection connection) {
        Intent intent = new Intent();
        intent.setClassName("com.google.tango", "com.google.atap.tango.TangoService");
        
        boolean hasJavaService = (context.getPackageManager().resolveService(intent, 0) != null);
        
        // User doesn't have the latest packagename for TangoCore, fallback to the previous name.
        if (!hasJavaService) {
            intent = new Intent();
            intent.setClassName("com.google.tango", "com.google.atap.tango.TangoService");
            hasJavaService = (context.getPackageManager().resolveService(intent, 0) != null);
        }
        
        // User doesn't have a Java-fied TangoCore at all; fallback to the deprecated approach
        // of doing nothing and letting the native side auto-init to the system-service version
        // of Tango.
        if (!hasJavaService) {
            return false;
        }
        
        return context.bindService(intent, connection, Context.BIND_AUTO_CREATE);
    }
    
    
    /**
     * Load the libtango_client_api.so library based on different Tango device setup.
     *
     * @return returns the loaded architecture id.
     */
    public static final int loadTangoSharedLibrary() {
        Log.e("TangoLifecycleDebugging", "Now attempting to load the shared library.");
        int loadedSoId = ARCH_ERROR;
        String basePath = "/data/data/com.google.tango/libfiles/";
        if (!(new File(basePath).exists())) {
            basePath = "/data/data/com.projecttango.tango/libfiles/";
        }
        Log.i("TangoInitializationHelper", "basePath: " + basePath);
        
        try {
            System.load(basePath + "arm64-v8a/libtango_client_api.so");
            loadedSoId = ARCH_ARM64;
            Log.i("TangoInitializationHelper", "Success! Using arm64-v8a/libtango_client_api.");
        } catch (UnsatisfiedLinkError e) {
        }
        if (loadedSoId < ARCH_DEFAULT) {
            try {
                System.load(basePath + "armeabi-v7a/libtango_client_api.so");
                loadedSoId = ARCH_ARM32;
                Log.i("TangoInitializationHelper", "Success! Using armeabi-v7a/libtango_client_api.");
            } catch (UnsatisfiedLinkError e) {
            }
        }
        if (loadedSoId < ARCH_DEFAULT) {
            try {
                System.load(basePath + "x86_64/libtango_client_api.so");
                loadedSoId = ARCH_X86_64;
                Log.i("TangoInitializationHelper", "Success! Using x86_64/libtango_client_api.");
            } catch (UnsatisfiedLinkError e) {
            }
        }
        if (loadedSoId < ARCH_DEFAULT) {
            try {
                System.load(basePath + "x86/libtango_client_api.so");
                loadedSoId = ARCH_X86;
                Log.i("TangoInitializationHelper", "Success! Using x86/libtango_client_api.");
            } catch (UnsatisfiedLinkError e) {
            }
        }
        if (loadedSoId < ARCH_DEFAULT) {
            try {
                System.load(basePath + "default/libtango_client_api.so");
                loadedSoId = ARCH_DEFAULT;
                Log.i("TangoInitializationHelper", "Success! Using default/libtango_client_api.");
            } catch (UnsatisfiedLinkError e) {
            }
        }
        if (loadedSoId < ARCH_DEFAULT) {
            try {
                System.loadLibrary("tango_client_api");
                loadedSoId = ARCH_FALLBACK;
                Log.i("TangoInitializationHelper", "Falling back to libtango_client_api.so symlink.");
            } catch (UnsatisfiedLinkError e) {
            }
        }
        return loadedSoId;
    }

}
