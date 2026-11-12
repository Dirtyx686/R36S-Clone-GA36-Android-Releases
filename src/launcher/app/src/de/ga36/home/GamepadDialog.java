package de.ga36.home;
import android.app.*;import android.content.*;import android.view.*;
final class GamepadDialog {
 static java.lang.ref.WeakReference<Dialog> visible;
 static void dismissVisible(){Dialog d=visible==null?null:visible.get();if(d!=null&&d.isShowing())d.dismiss();}
 static int mapKey(int key){return key==96?KeyEvent.KEYCODE_DPAD_CENTER:key==97?KeyEvent.KEYCODE_BACK:key;}
 static void attach(final Dialog dialog){visible=new java.lang.ref.WeakReference<Dialog>(dialog);dialog.setOnKeyListener(new DialogInterface.OnKeyListener(){public boolean onKey(DialogInterface d,int key,KeyEvent e){int mapped=mapKey(key);if(mapped==key)return false;return dialog.dispatchKeyEvent(new KeyEvent(e.getDownTime(),e.getEventTime(),e.getAction(),mapped,e.getRepeatCount(),e.getMetaState(),e.getDeviceId(),e.getScanCode(),e.getFlags(),e.getSource()));}});}
 static void show(AlertDialog dialog){attach(dialog);dialog.show();}
}
