package de.ga36.home;
import android.content.res.Resources;import android.graphics.*;import android.graphics.drawable.*;
final class GameCover {
 static Drawable create(Resources res,String pkg,Drawable source){
  Bitmap b=Bitmap.createBitmap(96,96,Bitmap.Config.ARGB_8888);Canvas c=new Canvas(b);Paint p=new Paint(3);int kind=GamePackages.coverKind(pkg);
  if(source!=null){int iw=Math.max(1,source.getIntrinsicWidth()),ih=Math.max(1,source.getIntrinsicHeight());float fit=Math.min(96f/iw,96f/ih);int w=Math.max(1,Math.round(iw*fit)),h=Math.max(1,Math.round(ih*fit));source.setBounds((96-w)/2,(96-h)/2,(96+w)/2,(96+h)/2);source.draw(c);return new BitmapDrawable(res,b);}
  int[] bg={0xff233d50,0xff173d55,0xff573d28,0xff302943,0xff173d48,0xff354155,0xff4a2d50,0xff1c3d5b,0xff3c3434,0xff243e42};p.setColor(bg[kind]);c.drawRoundRect(new RectF(0,0,96,96),16,16,p);
  if(kind==1){p.setColor(0xff89e6df);Path a=new Path();a.moveTo(48,12);a.lineTo(69,62);a.lineTo(48,54);a.lineTo(27,62);a.close();c.drawPath(a,p);p.setColor(0xffe9f5f9);c.drawCircle(48,34,6,p);p.setColor(0xffffc66b);c.drawRect(43,63,53,83,p);}
  if(kind==2){p.setColor(0xff9d6c38);for(int y=62;y<96;y+=14)for(int x=0;x<96;x+=24)c.drawRect(x+2,y+2,x+22,y+12,p);p.setColor(0xffe5d1a2);c.save();c.rotate(30,48,42);c.drawRect(44,20,52,69,p);p.setColor(0xff8ee1cb);c.drawRoundRect(new RectF(33,10,63,29),5,5,p);c.restore();}
  if(kind==3){p.setColor(0xfff4dda2);c.drawCircle(34,29,18,p);p.setColor(bg[kind]);c.drawCircle(44,22,18,p);p.setColor(0xff745b9f);c.drawRect(46,44,81,86,p);p.setColor(0xffe9ce7d);c.drawRect(55,55,62,64,p);c.drawRect(68,70,75,79,p);}
  if(kind==4){p.setColor(0xff76cbd5);c.drawRect(15,40,34,81,p);c.drawRect(38,20,58,81,p);c.drawRect(63,48,82,81,p);p.setColor(0xffe7efc5);for(int y=29;y<75;y+=15)c.drawRect(44,y,51,y+6,p);p.setColor(0xff4f8b8b);c.drawRect(8,84,88,91,p);}
  if(kind>=5||kind==0){p.setColor(0xff8bcbd7);c.drawRoundRect(new RectF(25,18,71,80),11,11,p);p.setColor(0xff203342);c.drawRoundRect(new RectF(30,29,66,48),5,5,p);c.drawRect(31,58,65,68,p);p.setColor(0xffe9d18c);c.drawRect(29,20,38,25,p);c.drawRect(58,20,67,25,p);p.setColor(0xff101b29);c.drawRect(20,30,25,45,p);c.drawRect(71,30,76,45,p);c.drawRect(20,61,25,75,p);c.drawRect(71,61,76,75,p);}
  return new BitmapDrawable(res,b);
 }
}
