package de.ga36.home;
import android.content.pm.*;import android.content.res.Resources;import android.graphics.drawable.Drawable;
/** Load APK resources only when this title or cover needs them. */
final class GameAssets {
 static String label(String pkg,ResolveInfo info,PackageManager pm){
  String known=GamePackages.knownTitle(pkg);if(known!=null)return known;
  CharSequence value=info.loadLabel(pm);String label=value==null?"":value.toString().trim();
  return label.isEmpty()?("com.nightshift.r36".equals(pkg)?"NightShift":pkg):label;
 }
 static Drawable cover(Resources res,String pkg,ResolveInfo info,PackageManager pm){
  Drawable logo=GameLogos.load(res,pkg);if(logo!=null)return GameCover.create(res,pkg,logo);
  return GameCover.create(res,pkg,GamePackages.coverKind(pkg)==0?info.loadIcon(pm):null);
 }
}
