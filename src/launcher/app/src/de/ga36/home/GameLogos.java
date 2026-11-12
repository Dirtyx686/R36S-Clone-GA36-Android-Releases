package de.ga36.home;
import android.content.res.Resources;import android.graphics.*;import android.graphics.drawable.*;
final class GameLogos {
 static String name(String pkg){
  if("com.pastagames.ro1mobile".equals(pkg))return "ga36_logo_com_pastagames_ro1mobile";
  if("nova.snowrunner".equals(pkg))return "ga36_logo_nova_snowrunner";
  if("durmus.mobilegame".equals(pkg))return "ga36_logo_durmus_mobilegame";
  if("com.gameloft.android.ANMP.GloftRAHM".equals(pkg))return "ga36_logo_com_gameloft_android_anmp_gloftrahm";
  if("com.ea.nfsmw_row".equals(pkg))return "ga36_logo_com_ea_nfsmw_row";
  if("com.gameloft.android.ANMP.GloftMOHM".equals(pkg))return "ga36_logo_com_gameloft_android_anmp_gloftmohm";
  if("com.gameloft.android.ANMP.GloftAGHM".equals(pkg))return "ga36_logo_com_gameloft_android_anmp_gloftaghm";
  if("com.giantssoftware.fs14".equals(pkg))return "ga36_logo_com_giantssoftware_fs14";
  if("com.ea.game.nfs14_row".equals(pkg))return "ga36_logo_com_ea_game_nfs14_row";
  if("com.ea.games.simsfreeplay_row".equals(pkg))return "ga36_logo_com_ea_games_simsfreeplay_row";
  if("com.ea.game.simcitymobile_row".equals(pkg))return "ga36_logo_com_ea_game_simcitymobile_row";
  if("com.gta4.cgs".equals(pkg))return "ga36_logo_com_gta4_cgs";
  if("com.NGC.GTAV".equals(pkg))return "ga36_logo_com_ngc_gtav";
  if("de.simcitybrowser.app.r36s".equals(pkg))return "ga36_logo_de_simcitybrowser_app_r36s";
  if("com.rockstargames.gtavc".equals(pkg))return "ga36_logo_com_rockstargames_gtavc";
  return null;
 }
 static Drawable load(Resources resources,String pkg){String name=name(pkg);if(name==null)return null;int id=resources.getIdentifier(name,"drawable","de.ga36.home");if(id==0)return null;BitmapFactory.Options size=new BitmapFactory.Options();size.inJustDecodeBounds=true;BitmapFactory.decodeResource(resources,id,size);BitmapFactory.Options decode=new BitmapFactory.Options();decode.inScaled=false;decode.inSampleSize=1;while(Math.max(size.outWidth,size.outHeight)/decode.inSampleSize>256)decode.inSampleSize*=2;Bitmap bitmap=BitmapFactory.decodeResource(resources,id,decode);return bitmap==null?null:new BitmapDrawable(resources,bitmap);}
}
