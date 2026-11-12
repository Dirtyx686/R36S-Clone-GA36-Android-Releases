package de.ga36.home;
import android.app.Activity;import android.os.Bundle;import android.os.Handler;import android.view.View;import android.view.WindowManager;import android.util.Log;
/** Console window policy; physical BACK/HOME/POWER stay enabled. */
public abstract class FullscreenActivity extends Activity {
 private final Handler fullscreenHandler=new Handler();private boolean active=false;private Object statusBar;
 private static final int FLAGS=View.SYSTEM_UI_FLAG_FULLSCREEN|View.SYSTEM_UI_FLAG_HIDE_NAVIGATION|View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY|View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN|View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION|View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
 private final Runnable restore=new Runnable(){public void run(){if(active&&getWindow().getDecorView().hasWindowFocus())applyFullscreen();}};
 protected void onCreate(Bundle saved){super.onCreate(saved);getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN|WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN);getWindow().getDecorView().setFitsSystemWindows(false);getWindow().getDecorView().setOnSystemUiVisibilityChangeListener(new View.OnSystemUiVisibilityChangeListener(){public void onSystemUiVisibilityChange(int visibility){if(active&&(visibility&FLAGS)!=FLAGS){fullscreenHandler.removeCallbacks(restore);fullscreenHandler.postDelayed(restore,80);}}});applyFullscreen();}
 private void applyFullscreen(){getWindow().getDecorView().setSystemUiVisibility(FLAGS);}
 protected final void collapseStatusPanel(){try{if(statusBar==null)statusBar=getSystemService("statusbar");if(statusBar!=null)statusBar.getClass().getMethod("collapsePanels").invoke(statusBar);}catch(Exception e){Log.w("GA36Fullscreen","Cannot collapse status panel",e);}}
 private void lockStatusPanel(){try{statusBar=getSystemService("statusbar");if(statusBar!=null){statusBar.getClass().getMethod("disable",Integer.TYPE).invoke(statusBar,0x00010000);Log.i("GA36Fullscreen","STATUS_BAR_EXPANSION_DISABLED; physical navigation unchanged");}}catch(Exception e){Log.e("GA36Fullscreen","Status panel permission not granted",e);}}
 protected void onResume(){super.onResume();active=true;collapseStatusPanel();applyFullscreen();lockStatusPanel();fullscreenHandler.removeCallbacks(restore);fullscreenHandler.postDelayed(restore,300);}
 protected void onPause(){active=false;fullscreenHandler.removeCallbacks(restore);super.onPause();}
 public void onWindowFocusChanged(boolean focused){super.onWindowFocusChanged(focused);if(focused){applyFullscreen();if(active){fullscreenHandler.removeCallbacks(restore);fullscreenHandler.postDelayed(restore,80);}}}
 protected void onDestroy(){fullscreenHandler.removeCallbacksAndMessages(null);super.onDestroy();}
}
