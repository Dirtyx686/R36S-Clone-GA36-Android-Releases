package de.ga36.home;
import java.util.Set;
final class BackgroundPolicy {
 static boolean eligible(String pkg,int flags,Set<String> protectedPackages){if(pkg==null||protectedPackages.contains(pkg)||"android".equals(pkg)||"com.android.systemui".equals(pkg)||"de.ga36.home".equals(pkg)||(flags&8)!=0)return false;boolean console="com.android.settings".equals(pkg)||"com.amaze.filemanager".equals(pkg)||"com.android.documentsui".equals(pkg)||GamePackages.isKnown(pkg);return console||(flags&(1|128))==0;}
}
