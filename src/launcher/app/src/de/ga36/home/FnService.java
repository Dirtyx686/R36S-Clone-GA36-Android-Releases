package de.ga36.home;
import android.accessibilityservice.*;import android.view.*;import android.view.accessibility.*;import android.content.*;import android.os.*;import android.provider.Settings;import android.widget.Toast;
public final class FnService extends AccessibilityService {
 final Handler chordHandler=new Handler();final BackgroundChord chord=new BackgroundChord();boolean modifier;String protectedAtPress;
 final Runnable closeHeld=new Runnable(){public void run(){if(chord.fire(SystemClock.uptimeMillis()))BackgroundTasks.close(FnService.this,protectedAtPress);}};
 void resetChord(){chordHandler.removeCallbacks(closeHeld);chord.reset();modifier=false;}
 int keyTrace;
 final FnKeys keys=new FnKeys();String foreground;long lastBrightness;boolean registered;
 final BroadcastReceiver screen=new BroadcastReceiver(){public void onReceive(Context c,Intent i){keys.reset();resetChord();}};
 protected void onServiceConnected(){AccessibilityServiceInfo info=getServiceInfo();info.flags|=AccessibilityServiceInfo.FLAG_REQUEST_FILTER_KEY_EVENTS;setServiceInfo(info);registerReceiver(screen,new IntentFilter(Intent.ACTION_SCREEN_OFF));registered=true;android.util.Log.i("GA36Keys","FN_FILTER_READY");}
 public void onAccessibilityEvent(AccessibilityEvent event){CharSequence p=event.getPackageName();if(p==null)return;String pkg=p.toString();if(!"android".equals(pkg)&&!"com.android.systemui".equals(pkg))BackgroundTasks.foreground=pkg;if(!"android".equals(pkg)&&!"com.android.systemui".equals(pkg))foreground=GameActionsActivity.game(this,pkg)?pkg:null;}
 public void onInterrupt(){keys.reset();resetChord();}
 public void onDestroy(){if(registered)unregisterReceiver(screen);keys.reset();resetChord();super.onDestroy();}
 protected boolean onKeyEvent(KeyEvent e){if(e.getKeyCode()==119){modifier=e.getAction()==KeyEvent.ACTION_DOWN;chordHandler.removeCallbacks(closeHeld);chord.reset();}
  InputDevice device=e.getDevice();if(device!=null&&keyTrace<128&&("micro_gamepad".equals(device.getName())||"udt_keyboard".equals(device.getName()))){keyTrace++;android.util.Log.i("GA36Keys","FILTER_KEY scan="+e.getScanCode()+" key="+e.getKeyCode()+" action="+e.getAction()+" fn="+modifier);}if(!modifier&&device!=null&&"micro_gamepad".equals(device.getName())&&(e.getScanCode()==307||e.getScanCode()==308)){boolean wasArmed=chord.since>=0;boolean consume=chord.key(e.getScanCode(),e.getAction()==KeyEvent.ACTION_DOWN,SystemClock.uptimeMillis());if(!wasArmed&&chord.since>=0)protectedAtPress=BackgroundTasks.foreground;chordHandler.removeCallbacks(closeHeld);long delay=chord.delay(SystemClock.uptimeMillis());if(delay>=0)chordHandler.postDelayed(closeHeld,delay);if(consume)return true;}
  int action=keys.key(e.getKeyCode(),e.getAction()==KeyEvent.ACTION_DOWN,e.getRepeatCount());if(action==FnKeys.PASS)return false;if(action==FnKeys.CONSUME)return true;
  if(action==FnKeys.BRIGHTER||action==FnKeys.DARKER){long now=SystemClock.uptimeMillis();if(e.getRepeatCount()>0&&now-lastBrightness<150)return true;lastBrightness=now;try{int value=Settings.System.getInt(getContentResolver(),Settings.System.SCREEN_BRIGHTNESS,128);Settings.System.putInt(getContentResolver(),Settings.System.SCREEN_BRIGHTNESS_MODE,Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL);Settings.System.putInt(getContentResolver(),Settings.System.SCREEN_BRIGHTNESS,Math.max(20,Math.min(255,value+(action==FnKeys.BRIGHTER?20:-20))));}catch(RuntimeException ex){android.util.Log.e("GA36Keys","BRIGHTNESS_FAILED",ex);}return true;}
  if(action==FnKeys.HOME){startActivity(new Intent(this,HomeActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK|Intent.FLAG_ACTIVITY_CLEAR_TOP));return true;}
  if(action==FnKeys.SAVE||action==FnKeys.LOAD){Toast.makeText(this,I18n.t("Speichern und Laden ist nur im unterstützten Emulator möglich"),0).show();return true;}
  Intent menu=new Intent(this,GameActionsActivity.class).putExtra("game",foreground).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
  if(action==FnKeys.RESTART){if(foreground==null){Toast.makeText(this,I18n.t("Kein laufendes Spiel erkannt"),0).show();return true;}menu.putExtra("command","restart");}
  startActivity(menu);return true;
 }
}
