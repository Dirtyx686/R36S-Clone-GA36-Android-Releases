package de.ga36.home;
import android.app.*;import android.os.*;import android.content.*;import android.content.pm.*;import android.graphics.*;import android.graphics.drawable.*;import android.view.*;import android.widget.*;import android.provider.Settings;import java.util.*;import java.text.*;
public class HomeActivity extends FullscreenActivity {
 static final String[] THEMES={I18n.t("Xbox Grün"),I18n.t("Violett"),I18n.t("Hell"),I18n.t("Blau"),I18n.t("Koralle"),I18n.t("Wald"),"Gold",I18n.t("Ozean")};
 final ArrayList<Game> candidates=new ArrayList<Game>(),games=new ArrayList<Game>();final CatalogState catalog=new CatalogState();final Handler ui=new Handler();final java.util.concurrent.ExecutorService worker=java.util.concurrent.Executors.newSingleThreadExecutor();
 Board board;int pageSize=4,theme=0,selection=0,page=0,tabFocus=-1,battery=-1;boolean destroyed=false,dashboard=true,navigating;int stickKey;
 static final class Game extends XboxRenderer.Tile{final ComponentName component;Game(String l,ComponentName c,Drawable i,Drawable art){super(l,i,art);component=c;}}
 final Runnable clock=new Runnable(){public void run(){if(board!=null)board.invalidate();ui.postDelayed(this,30000);}};
 final BroadcastReceiver batteryReceiver=new BroadcastReceiver(){public void onReceive(Context c,Intent i){int n=i.getIntExtra(BatteryManager.EXTRA_LEVEL,-1),max=i.getIntExtra(BatteryManager.EXTRA_SCALE,100);battery=n<0?-1:n*100/Math.max(1,max);if(board!=null)board.invalidate();}};
 final BroadcastReceiver packages=new BroadcastReceiver(){public void onReceive(Context c,Intent i){catalog.invalidate();refresh();}};
 public void onCreate(Bundle state){super.onCreate(state);android.util.Log.i("GA36Home","HOME_BUILD=XBOX_P38");pageSize=GameLayout.validSize(getPreferences(MODE_PRIVATE).getInt("tiles",4));theme=getPreferences(MODE_PRIVATE).getInt("theme",0);if(theme<0||theme>=THEMES.length)theme=0;if(state!=null){dashboard=state.getBoolean("dashboard",true);page=Math.max(0,state.getInt("page",0));selection=Math.max(0,state.getInt("selection",0));}board=new Board();setContentView(board);registerReceiver(batteryReceiver,new IntentFilter(Intent.ACTION_BATTERY_CHANGED));IntentFilter f=new IntentFilter();for(String action:new String[]{Intent.ACTION_PACKAGE_ADDED,Intent.ACTION_PACKAGE_REMOVED,Intent.ACTION_PACKAGE_CHANGED,Intent.ACTION_PACKAGE_REPLACED})f.addAction(action);f.addDataScheme("package");registerReceiver(packages,f);}
 protected void onResume(){super.onResume();overridePendingTransition(0,0);navigating=true;refresh();applyFilter();ui.removeCallbacks(clock);ui.post(clock);board.requestFocus();}
 protected void onPause(){navigating=false;resetStick();ui.removeCallbacks(clock);super.onPause();}
 protected void onDestroy(){destroyed=true;ui.removeCallbacksAndMessages(null);worker.shutdownNow();unregisterReceiver(packages);unregisterReceiver(batteryReceiver);super.onDestroy();}
 protected void onSaveInstanceState(Bundle b){b.putInt("page",page);b.putInt("selection",selection);b.putBoolean("dashboard",dashboard);super.onSaveInstanceState(b);}
 static boolean candidate(String pkg,int flags){if("de.ga36.home".equals(pkg)||"com.amaze.filemanager".equals(pkg)||"com.android.settings".equals(pkg))return false;return GamePackages.isKnown(pkg)||(flags&0x02000000)!=0||(flags&ApplicationInfo.FLAG_SYSTEM)==0;}
 static String gameTitle(String pkg,String label){if("com.skyrunner.game".equals(pkg))return "SkyRunner";if("de.hackbook.diggis".equals(pkg))return "Diggis Abenteuer";if("com.nightshift.r36".equals(pkg))return label.isEmpty()?"NightShift":label;if("de.simcitybrowser.app.r36s".equals(pkg))return "SimCity Lite";return label.isEmpty()?pkg:label;}
 void refresh(){if(destroyed||!catalog.begin())return;worker.execute(new Runnable(){public void run(){android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_BACKGROUND);final ArrayList<Game> found=new ArrayList<Game>();boolean success=true;long begin=SystemClock.uptimeMillis();try{PackageManager pm=getPackageManager();HashSet<String> seen=new HashSet<String>();for(ResolveInfo r:pm.queryIntentActivities(new Intent(Intent.ACTION_MAIN).addCategory(Intent.CATEGORY_LAUNCHER),0)){if(Thread.currentThread().isInterrupted())return;String pkg=r.activityInfo.packageName;if(!candidate(pkg,r.activityInfo.applicationInfo.flags))continue;ComponentName c=new ComponentName(pkg,r.activityInfo.name);if(!seen.add(pkg))continue;try{found.add(new Game(gameTitle(pkg,GameAssets.label(pkg,r,pm)),c,GameAssets.cover(getResources(),pkg,r,pm),DashboardArt.load(getResources(),pkg)));}catch(RuntimeException changed){android.util.Log.w("GA36Home","Package changed during scan",changed);}}Collections.sort(found,new Comparator<Game>(){public int compare(Game a,Game b){return a.label.compareToIgnoreCase(b.label);}});}catch(RuntimeException e){success=false;android.util.Log.e("GA36Home","Game scan failed",e);}final boolean ok=success;android.util.Log.i("GA36Home","GAME_SCAN_MS="+(SystemClock.uptimeMillis()-begin)+" games="+found.size());ui.post(new Runnable(){public void run(){if(destroyed)return;catalog.complete(ok);if(ok){candidates.clear();candidates.addAll(found);applyFilter();if(catalog.isDirty())refresh();}}});}});}
 void applyFilter(){
  int absolute=page*slots()+selection;String selected=absolute>=0&&absolute<games.size()?games.get(absolute).component.flattenToString():null;
  games.clear();for(Game g:candidates)if(getPreferences(MODE_PRIVATE).getBoolean("game."+g.component.getPackageName(),true))games.add(g);
  String recent=getPreferences(MODE_PRIVATE).getString("lastPlayed","");
  for(int i=0;i<games.size();i++)if(games.get(i).component.flattenToString().equals(recent)){Game g=games.remove(i);games.add(0,g);break;}
  if(selected!=null)for(int i=0;i<games.size();i++)if(games.get(i).component.flattenToString().equals(selected)){page=i/slots();selection=i%slots();break;}
  page=Math.max(0,Math.min(page,Math.max(0,(games.size()-1)/slots())));selection=Math.max(0,Math.min(selection,Math.max(0,count()-1)));board.invalidate();
 }
 boolean hasRecent(){return !games.isEmpty()&&games.get(0).component.flattenToString().equals(getPreferences(MODE_PRIVATE).getString("lastPlayed",""));}
 int slots(){return XboxLayout.slots(dashboard,pageSize);}
 int count(){return Math.min(slots(),Math.max(0,games.size()-page*slots()));}
 void showMode(boolean home){dashboard=home;page=selection=0;tabFocus=-1;applyFilter();}
 int tone(int c){return ThemePalette.color(theme,c);}
 boolean launch(Intent i){collapseStatusPanel();try{ComponentName target=i.getComponent();boolean internal=target!=null&&getPackageName().equals(target.getPackageName());Intent request=new Intent(i);request.setFlags(LaunchPolicy.flags(internal,request.getFlags()));startActivity(request,ActivityOptions.makeCustomAnimation(this,0,0).toBundle());overridePendingTransition(0,0);return true;}catch(Exception e){android.util.Log.e("GA36Home","Activity launch failed: "+i.getComponent(),e);Toast.makeText(this,I18n.t("Nicht verfügbar"),Toast.LENGTH_LONG).show();catalog.invalidate();refresh();return false;}}
 void files(){Intent i=getPackageManager().getLaunchIntentForPackage("com.android.documentsui");if(i==null)i=getPackageManager().getLaunchIntentForPackage("com.amaze.filemanager");if(i!=null)launch(i);else Toast.makeText(this,I18n.t("Dateimanager nicht verfügbar"),1).show();}
 void settings(){launch(new Intent(Settings.ACTION_SETTINGS));}
 void topAction(int i){if(i==0)showMode(true);else if(i==1)showMode(false);else if(i==2)files();else if(i==3)settings();else if(i==4)menu();}
 void open(){
  if(tabFocus>=0){topAction(tabFocus);return;}
  int n=page*slots()+selection;
  if(n>=0&&n<games.size()){
   Game g=games.get(n);if(launch(new Intent(Intent.ACTION_MAIN).addCategory(Intent.CATEGORY_LAUNCHER).setComponent(g.component)))getPreferences(MODE_PRIVATE).edit().putString("lastPlayed",g.component.flattenToString()).apply();
  }
 }
 void menu(){GamepadDialog.show(new AlertDialog.Builder(this).setTitle(I18n.t("GA36 · Menü")).setItems(new String[]{I18n.t("Spiele auswählen"),I18n.t("Dateimanager"),I18n.t("Android-Einstellungen"),I18n.t("Design"),I18n.t("Ausschalten / Neustart"),I18n.t("Hintergrund-Apps schließen"),I18n.t("USB zum PC"),I18n.t("FN-Tasten")},new DialogInterface.OnClickListener(){public void onClick(DialogInterface d,int n){if(n==0)manage();if(n==1)files();if(n==2)settings();if(n==3)design();if(n==4)launch(new Intent(HomeActivity.this,PowerActivity.class));if(n==5)BackgroundTasks.close(HomeActivity.this,"de.ga36.home");if(n==6)launch(new Intent(HomeActivity.this,UsbActivity.class));if(n==7)launch(new Intent(HomeActivity.this,KeysActivity.class));}}).create());}
 boolean rebooting;
 void restartToSelection(){if(rebooting)return;rebooting=true;try{android.util.Log.i("GA36Boot","USER_RESTART_TO_PRE_ANDROID_MENU");((PowerManager)getSystemService(POWER_SERVICE)).reboot(null);}catch(Exception e){rebooting=false;android.util.Log.e("GA36Boot","Restart failed",e);Toast.makeText(this,I18n.t("Neustart fehlgeschlagen"),Toast.LENGTH_LONG).show();}}
 void design(){GamepadDialog.show(new AlertDialog.Builder(this).setTitle(I18n.t("Design & Ansicht")).setItems(new String[]{I18n.t("Farbschema · ")+THEMES[theme],I18n.t("Kachelansicht · ")+pageSize+I18n.t(" Spiele")},new DialogInterface.OnClickListener(){public void onClick(DialogInterface d,int n){if(n==0)themes();else tiles();}}).create());}
 void tiles(){final int[] sizes={2,4,6};int choice=pageSize==2?0:pageSize==6?2:1;GamepadDialog.show(new AlertDialog.Builder(this).setTitle(I18n.t("Spiele pro Seite")).setSingleChoiceItems(new String[]{I18n.t("2 Kacheln · große Ansicht"),I18n.t("4 Kacheln · Standard"),I18n.t("6 Kacheln · Übersicht")},choice,new DialogInterface.OnClickListener(){public void onClick(DialogInterface d,int n){int absolute=page*slots()+selection;pageSize=sizes[n];dashboard=false;page=absolute/pageSize;selection=absolute%pageSize;getPreferences(MODE_PRIVATE).edit().putInt("tiles",pageSize).apply();applyFilter();d.dismiss();}}).setNegativeButton(I18n.t("Zurück"),null).create());}
 void themes(){GamepadDialog.show(new AlertDialog.Builder(this).setTitle(I18n.t("Design")).setSingleChoiceItems(THEMES,theme,new DialogInterface.OnClickListener(){public void onClick(DialogInterface d,int n){theme=n;getPreferences(MODE_PRIVATE).edit().putInt("theme",n).apply();board.themeChanged();board.invalidate();d.dismiss();}}).setNegativeButton(I18n.t("Zurück"),null).create());}
 void manage(){if(candidates.isEmpty()){Toast.makeText(this,I18n.t("Noch keine Spiele gefunden"),1).show();return;}final ArrayList<Game> snapshot=new ArrayList<Game>(candidates);String[] labels=new String[snapshot.size()];final boolean[] checked=new boolean[snapshot.size()];for(int i=0;i<labels.length;i++){labels[i]=snapshot.get(i).label;checked[i]=getPreferences(MODE_PRIVATE).getBoolean("game."+snapshot.get(i).component.getPackageName(),true);}GamepadDialog.show(new AlertDialog.Builder(this).setTitle(I18n.t("Spiele auswählen")).setMultiChoiceItems(labels,checked,new DialogInterface.OnMultiChoiceClickListener(){public void onClick(DialogInterface d,int n,boolean yes){checked[n]=yes;}}).setPositiveButton(I18n.t("Speichern"),new DialogInterface.OnClickListener(){public void onClick(DialogInterface d,int n){android.content.SharedPreferences.Editor e=getPreferences(MODE_PRIVATE).edit();for(int i=0;i<checked.length;i++)e.putBoolean("game."+snapshot.get(i).component.getPackageName(),checked[i]);e.apply();page=selection=0;applyFilter();}}).setNegativeButton(I18n.t("Abbrechen"),null).create());}
 void navigate(int key){
  if(tabFocus>=0)tabFocus=XboxLayout.top(tabFocus,key);
  else if(key==19&&XboxLayout.firstRow(selection,dashboard,pageSize))tabFocus=dashboard?0:1;
  else{int[] pos=XboxLayout.move(selection,page,games.size(),key,dashboard,pageSize);selection=pos[0];page=pos[1];}
  board.invalidate();
 }
 final Runnable stickRepeat=new Runnable(){public void run(){if(navigating&&hasWindowFocus()&&stickKey!=0){navigate(stickKey);ui.postDelayed(this,180);}}};
 void resetStick(){stickKey=0;ui.removeCallbacks(stickRepeat);}
 public void onWindowFocusChanged(boolean focus){super.onWindowFocusChanged(focus);if(!focus)resetStick();}
 public boolean onGenericMotionEvent(MotionEvent e){
  if(navigating&&hasWindowFocus()&&(e.getSource()&InputDevice.SOURCE_JOYSTICK)==InputDevice.SOURCE_JOYSTICK&&e.getActionMasked()==MotionEvent.ACTION_MOVE){
   int next=XboxStick.direction(e.getAxisValue(MotionEvent.AXIS_X),e.getAxisValue(MotionEvent.AXIS_Y),stickKey);
   if(next!=stickKey){resetStick();stickKey=next;if(next!=0){navigate(next);ui.postDelayed(stickRepeat,400);}}return true;
  }
  return super.onGenericMotionEvent(e);
 }
 public boolean dispatchKeyEvent(KeyEvent e){
  int k=e.getKeyCode();
  if(k>=19&&k<=22){if(e.getAction()==KeyEvent.ACTION_DOWN)navigate(k);return true;}
  if(k==23||k==66||k==96){if(e.getAction()==KeyEvent.ACTION_DOWN&&e.getRepeatCount()==0)open();return true;}
  if(k==4||k==97){if(e.getAction()==KeyEvent.ACTION_DOWN&&e.getRepeatCount()==0){resetStick();tabFocus=tabFocus>=0?-1:(dashboard?0:1);board.invalidate();}return true;}
  if(k==82||k==108){if(e.getAction()==KeyEvent.ACTION_DOWN&&e.getRepeatCount()==0){resetStick();menu();}return true;}
  if(k==102||k==103){if(e.getAction()==KeyEvent.ACTION_DOWN&&e.getRepeatCount()==0){showMode(k==102);}return true;}
  return super.dispatchKeyEvent(e);
 }
 final class Board extends View{
  final XboxRenderer renderer=new XboxRenderer();final SimpleDateFormat clockFormat=new SimpleDateFormat("HH:mm",Locale.getDefault());
  long minute=-1;String time="";int pressed=XboxLayout.NONE;boolean ready;
  Board(){super(HomeActivity.this);setFocusable(true);setFocusableInTouchMode(true);setContentDescription(I18n.t("GA36 Spiele. Steuerkreuz oder linker Stick: Auswahl. A: Spielen. B: Menüleiste. START: Menü. L1: Start. R1: Spiele."));}
  void themeChanged(){invalidate();}
  protected void onDraw(Canvas canvas){
   long now=System.currentTimeMillis()/60000;if(now!=minute){minute=now;clockFormat.setTimeZone(TimeZone.getDefault());time=clockFormat.format(new Date());}
   renderer.paint(canvas,getWidth(),getHeight(),games,dashboard,pageSize,page,selection,tabFocus,theme,time,battery,catalog.isLoading(),hasRecent());
   if(!ready){ready=true;ui.post(new Runnable(){public void run(){try{java.io.FileOutputStream f=new java.io.FileOutputStream(new java.io.File(getFilesDir(),"ready-v1"));f.write("GA36_HOME_FIRST_DRAW_OK\n".getBytes("UTF-8"));f.close();}catch(Exception e){android.util.Log.e("GA36Home","Ready marker failed",e);}}});}
  }
  int hit(float x,float y){return XboxLayout.hit((x-renderer.ox)/renderer.scale,(y-renderer.oy)/renderer.scale,count(),dashboard,pageSize);}
  public boolean onTouchEvent(MotionEvent e){
   int h=hit(e.getX(),e.getY());
   if(e.getActionMasked()==MotionEvent.ACTION_DOWN){resetStick();pressed=h;if(h>=0){selection=h;tabFocus=-1;}else if(h<=XboxLayout.TOP_BASE)tabFocus=XboxLayout.TOP_BASE-h;invalidate();return true;}
   if(e.getActionMasked()==MotionEvent.ACTION_UP){if(h==pressed){if(h>=0)open();else if(h<=XboxLayout.TOP_BASE)topAction(XboxLayout.TOP_BASE-h);}pressed=XboxLayout.NONE;return true;}
   if(e.getActionMasked()==MotionEvent.ACTION_CANCEL)pressed=XboxLayout.NONE;return true;
  }
 }
}
