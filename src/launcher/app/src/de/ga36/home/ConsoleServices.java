package de.ga36.home;
import android.content.*;import android.provider.Settings;import android.content.pm.*;import android.os.Build;
final class ConsoleServices {
 static final String FN="de.ga36.home/de.ga36.home.FnService";
 static String withFn(String value){
  if(value==null||value.isEmpty()||"null".equals(value))return FN;
  for(String part:value.split(":"))if(FN.equals(part)||"de.ga36.home/.FnService".equals(part))return value;
  return value+":"+FN;
 }
 static void configure(Context c){
  String old=Settings.Secure.getString(c.getContentResolver(),"enabled_accessibility_services");String next=withFn(old);
  if(!next.equals(old)&&!Settings.Secure.putString(c.getContentResolver(),"enabled_accessibility_services",next))throw new IllegalStateException("FN registration failed");
  if(Settings.Secure.getInt(c.getContentResolver(),"accessibility_enabled",0)!=1&&!Settings.Secure.putInt(c.getContentResolver(),"accessibility_enabled",1))throw new IllegalStateException("FN activation failed");
  if(!next.equals(Settings.Secure.getString(c.getContentResolver(),"enabled_accessibility_services"))||Settings.Secure.getInt(c.getContentResolver(),"accessibility_enabled",0)!=1)throw new IllegalStateException("FN activation readback failed");
  configureConsolePackages(c);
  // USB is selected explicitly in its panel.

 }
 static void configureConsolePackages(Context c){
  if(Build.VERSION.SDK_INT<30||!"ga36".equals(Build.HARDWARE))return;
  android.content.SharedPreferences prefs=c.getSharedPreferences("system_start",0);
  if(prefs.getBoolean("console_packages_p39",false))return;
  if(c.checkSelfPermission("android.permission.CHANGE_COMPONENT_ENABLED_STATE")!=PackageManager.PERMISSION_GRANTED)return;
  PackageManager pm=c.getPackageManager();
  boolean phone=pm.hasSystemFeature(PackageManager.FEATURE_TELEPHONY);
  boolean secure=pm.hasSystemFeature("android.hardware.se.omapi.ese")||pm.hasSystemFeature("android.hardware.se.omapi.uicc")||pm.hasSystemFeature("android.hardware.se.omapi.sd");
  Intent home=new Intent(Intent.ACTION_MAIN).addCategory(Intent.CATEGORY_HOME);
  ResolveInfo resolved=pm.resolveActivity(home,PackageManager.MATCH_DEFAULT_ONLY);
  boolean own=resolved!=null&&resolved.activityInfo!=null&&c.getPackageName().equals(resolved.activityInfo.packageName);
  for(String pkg:new String[]{"com.android.phone","com.android.smspush","com.android.se","com.android.launcher3"}){
   if(!ConsolePackagePolicy.disable(pkg,phone,secure,own))continue;
   try{pm.getApplicationInfo(pkg,0);if(pm.getApplicationEnabledSetting(pkg)==PackageManager.COMPONENT_ENABLED_STATE_DEFAULT)pm.setApplicationEnabledSetting(pkg,PackageManager.COMPONENT_ENABLED_STATE_DISABLED_USER,0);}
   catch(PackageManager.NameNotFoundException absent){} // Board images may omit these apps.
  }
  // If HOME has not been resolved to GA36 yet, retry the stock-launcher decision next boot.
  if(own)prefs.edit().putBoolean("console_packages_p39",true).apply();
 }

}
