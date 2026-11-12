package de.ga36.home;
import android.content.res.Resources;import android.graphics.*;import android.graphics.drawable.Drawable;
/** One shared 768x256 RGB565 sheet: approximately 384 KiB for all three covers. */
final class DashboardArt extends Drawable {
 static Bitmap sheet;
 final Rect src;final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
 DashboardArt(Bitmap b,int index){int width=b.getWidth()/3;src=new Rect(index*width,0,(index+1)*width,b.getHeight());}
 static synchronized Drawable load(Resources res,String pkg){
  int index="com.rockstargames.gtavc".equals(pkg)?0:"com.skyrunner.game".equals(pkg)?1:("de.simcitybrowser.app.r36s".equals(pkg)||"com.ea.game.simcitymobile_row".equals(pkg))?2:-1;
  if(index<0)return null;
  if(sheet==null){int id=res.getIdentifier("ga36_dashboard_art","drawable","de.ga36.home");if(id==0)return null;BitmapFactory.Options options=new BitmapFactory.Options();options.inScaled=false;options.inPreferredConfig=Bitmap.Config.RGB_565;sheet=BitmapFactory.decodeResource(res,id,options);}
  return sheet==null?null:new DashboardArt(sheet,index);
 }
 public void draw(Canvas c){c.drawBitmap(sheet,src,getBounds(),paint);}
 public int getIntrinsicWidth(){return src.width();}public int getIntrinsicHeight(){return src.height();}
 public void setAlpha(int value){paint.setAlpha(value);invalidateSelf();}
 public void setColorFilter(ColorFilter filter){paint.setColorFilter(filter);invalidateSelf();}
 public int getOpacity(){return PixelFormat.OPAQUE;}
}
