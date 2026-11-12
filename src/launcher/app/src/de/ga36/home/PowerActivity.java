package de.ga36.home;
import android.os.*;import android.content.*;import android.view.*;import android.widget.*;
public final class PowerActivity extends FullscreenActivity {
 PowerScreen screen;boolean acting,screenReceiverRegistered;
 final BroadcastReceiver screenOff=new BroadcastReceiver(){public void onReceive(Context c,Intent i){finish();}};
 public void onCreate(Bundle state){super.onCreate(state);getWindow().addFlags(WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED);
  if(checkCallingOrSelfPermission("android.permission.REBOOT")!=android.content.pm.PackageManager.PERMISSION_GRANTED){Toast.makeText(this,I18n.t("Neustart-Berechtigung fehlt"),1).show();finish();return;}
  screen=new PowerScreen(this,new PowerScreen.Actions(){public void choose(int index){select(index);}});setContentView(screen);registerReceiver(screenOff,new IntentFilter(Intent.ACTION_SCREEN_OFF));screenReceiverRegistered=true;
  android.util.Log.i("GA36Power","CUSTOM_POWER_SCREEN_CREATED_V63");
 }
 protected void onResume(){super.onResume();if(screen!=null)screen.requestFocus();}
 protected void onNewIntent(Intent intent){super.onNewIntent(intent);if(!acting&&screen!=null){screen.selected=PowerLayout.CANCEL;screen.invalidate();}android.util.Log.i("GA36Power","CUSTOM_POWER_SCREEN_REUSED");}
 public boolean dispatchKeyEvent(KeyEvent e){int key=e.getKeyCode();if(key>=19&&key<=22){if(e.getAction()==0&&!acting&&screen!=null){screen.selected=PowerLayout.move(screen.selected,key);screen.invalidate();}return true;}if(key==23||key==66||key==96){if(e.getAction()==0&&e.getRepeatCount()==0&&screen!=null)select(screen.selected);return true;}if(key==4||key==97){if(e.getAction()==0&&e.getRepeatCount()==0&&!acting)finish();return true;}return super.dispatchKeyEvent(e);}
 void select(int index){if(acting)return;if(index==PowerLayout.CANCEL){finish();return;}if(index>=0&&index<2)power(index==1);}
 void power(boolean restart){if(acting)return;acting=true;try{PowerManager pm=(PowerManager)getSystemService(POWER_SERVICE);android.util.Log.i("GA36Power",restart?"USER_RESTART_SELECTED":"USER_SHUTDOWN_SELECTED");if(restart)pm.reboot(null);else PowerManager.class.getMethod("shutdown",boolean.class,String.class,boolean.class).invoke(pm,false,null,false);finish();}catch(Exception error){acting=false;android.util.Log.e("GA36Power","Power action failed",error);Toast.makeText(this,I18n.t("Aktion fehlgeschlagen"),1).show();}}
 public void finish(){finishAndRemoveTask();}
 protected void onDestroy(){if(screenReceiverRegistered)unregisterReceiver(screenOff);super.onDestroy();}
}
