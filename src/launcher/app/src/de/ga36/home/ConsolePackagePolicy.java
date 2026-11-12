package de.ga36.home;
final class ConsolePackagePolicy {
 static boolean disable(String pkg,boolean phone,boolean secureElement,boolean ownHome){
  if("com.android.phone".equals(pkg)||"com.android.smspush".equals(pkg))return !phone;
  if("com.android.se".equals(pkg))return !secureElement;
  return "com.android.launcher3".equals(pkg)&&ownHome;
 }
}
