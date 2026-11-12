package de.ga36.home;
import android.app.Application;import android.content.*;import android.content.pm.PackageManager;import android.os.*;
public final class GA36Application extends Application {
 private BroadcastReceiver powerReceiver;private final PowerMenuDispatch dispatch=new PowerMenuDispatch();private final Handler handler=new Handler();
 public void onCreate(){super.onCreate();
  if(checkCallingOrSelfPermission("android.permission.REBOOT")!=PackageManager.PERMISSION_GRANTED){android.util.Log.e("GA36Power","REBOOT_PERMISSION_MISSING; custom power unavailable");return;}
  powerReceiver=new BroadcastReceiver(){public void onReceive(Context c,Intent i){if(!PowerMenuPolicy.accepts(i.getAction(),i.getStringExtra("reason"))||!dispatch.begin())return;
   android.util.Log.i("GA36Power","LONG_POWER_MENU_REQUEST closing_standard_first uptime="+SystemClock.uptimeMillis());
   try{sendOrderedBroadcast(new Intent(Intent.ACTION_CLOSE_SYSTEM_DIALOGS).putExtra("reason","ga36power"),null,new BroadcastReceiver(){public void onReceive(Context context,Intent intent){
    PowerManager pm=(PowerManager)getSystemService(POWER_SERVICE);boolean interactive=pm!=null&&pm.isInteractive();
    if(!dispatch.complete(interactive)){android.util.Log.i("GA36Power","POWER_MENU_CANCELLED_SCREEN_OFF");return;}
    android.util.Log.i("GA36Power","STANDARD_CLOSE_DELIVERED uptime="+SystemClock.uptimeMillis());
    try{startActivity(new Intent(GA36Application.this,PowerActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK|Intent.FLAG_ACTIVITY_CLEAR_TOP|Intent.FLAG_ACTIVITY_SINGLE_TOP));}catch(RuntimeException e){android.util.Log.e("GA36Power","Power menu launch failed",e);}
   }},handler,0,null,null);}catch(RuntimeException e){dispatch.cancel();android.util.Log.e("GA36Power","Ordered close unavailable; custom power launch failed",e);}
  }};
  registerReceiver(powerReceiver,new IntentFilter(Intent.ACTION_CLOSE_SYSTEM_DIALOGS));android.util.Log.i("GA36Power","POWER_MENU_REGISTERED native_globalactions_ui_disabled_V63");setupConsole(0);
 }
 private void setupConsole(final int attempt){new Thread(new Runnable(){public void run(){try{ConsoleServices.configure(GA36Application.this);android.util.Log.i("GA36Console","FN_AND_PC_SERVICES_CONFIGURED");}catch(RuntimeException e){android.util.Log.e("GA36Console","Console services setup failed",e);if(attempt<2)handler.postDelayed(new Runnable(){public void run(){setupConsole(attempt+1);}},1500);}}},"GA36 setup").start();}
}
