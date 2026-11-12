package de.ga36.home;
import android.graphics.*;import android.graphics.drawable.Drawable;import java.util.List;
/** Event-driven Canvas renderer: no render thread, wallpaper service or continuous animation. */
final class XboxRenderer {
 static class Tile {final String label;final Drawable icon,art;Tile(String l,Drawable i){this(l,i,null);}Tile(String l,Drawable i,Drawable a){label=l;icon=i;art=a;}}
 final Paint p=new Paint(3);final RectF bounds=new RectF();
 final Typeface regular=Typeface.create("sans-serif",0),bold=Typeface.create("sans-serif",1);
 Canvas c;int theme;float scale,ox,oy;
 int color(int value){return ThemePalette.color(theme,value);}
 void box(float x,float y,float w,float h,int color,int stroke){
  p.setShader(null);p.setColor(color);p.setStyle(stroke==0?Paint.Style.FILL:Paint.Style.STROKE);p.setStrokeWidth(stroke);
  bounds.set(x,y,x+w,y+h);c.drawRoundRect(bounds,8,8,p);p.setStyle(Paint.Style.FILL);
 }
 void text(String value,float x,float y,float size,int color,boolean strong){
  p.setShader(null);p.setColor(color);p.setTypeface(strong?bold:regular);p.setTextSize(size);c.drawText(value,x,y,p);
 }
 String fit(String value,float width,float size){
  p.setTypeface(bold);p.setTextSize(size);if(p.measureText(value)<=width)return value;
  int n=p.breakText(value,true,Math.max(0,width-p.measureText("…")),null);
  if(n>0&&n<value.length()&&Character.isHighSurrogate(value.charAt(n-1)))n--;
  return value.substring(0,n)+"…";
 }
 void title(String value,float x,float y,float width,float size){
  p.setTypeface(bold);p.setTextSize(size);int n=p.breakText(value,true,width,null);
  if(n>=value.length()){text(value,x,y,size,0xfff6f7f5,true);return;}
  int space=value.lastIndexOf(' ',n);if(space>0)n=space;
  if(n>0&&Character.isHighSurrogate(value.charAt(n-1)))n--;
  text(value.substring(0,n),x,y,size,0xfff6f7f5,true);
  text(fit(value.substring(n).trim(),width,size),x,y+size+3,size,0xfff6f7f5,true);
 }
 void paint(Canvas canvas,int width,int height,List<? extends Tile> games,boolean home,int pageSize,int page,int selection,int topFocus,int scheme,String time,int battery,boolean scanning,boolean recent){
  c=canvas;theme=scheme;scale=Math.min(width/640f,height/480f);ox=(width-640*scale)/2;oy=(height-480*scale)/2;
  c.drawColor(color(0xff0a111d));c.save();c.translate(ox,oy);c.scale(scale,scale);
  int accent=ThemePalette.PRIMARY[Math.max(0,Math.min(7,theme))],fg=color(0xffe9f0f8),muted=color(0xffa5b7c8);
  box(0,0,640,126,color(0xff101a2b),0);box(24,24,30,30,accent,0);
  text("+",31,47,23,0xff0a1606,true);text("GA36",65,48,25,fg,true);if(battery>=0)text(battery+"%",496,46,12,muted,false);text(time,557,46,16,fg,true);
  String[] tabs={I18n.t("Start"),I18n.t("Spiele"),I18n.t("Dateien"),I18n.t("Einstellungen"),I18n.t("Menü")};
  for(int i=0;i<5;i++){
   boolean active=(home?0:1)==i;int x=XboxLayout.TOP_X[i],w=XboxLayout.TOP_W[i];
   box(x,82,w,34,active?accent:color(0xff1b293a),0);
   if(topFocus==i)box(x-2,80,w+4,38,accent,2);
   text(tabs[i],x+(i==3?12:14),105,14,active?0xff0b1b06:fg,active);
  }
  text(home?(recent?I18n.t("Zuletzt gespielt"):I18n.t("Deine Spiele")):I18n.t("Spielebibliothek"),24,153,22,fg,true);
  int slots=XboxLayout.slots(home,pageSize),count=Math.min(slots,Math.max(0,games.size()-page*slots));
  text((page+1)+" / "+Math.max(1,(games.size()+slots-1)/slots),565,152,12,muted,false);
  for(int i=0;i<count;i++){
   Tile g=games.get(page*slots+i);int[] r=XboxLayout.rect(i,home,pageSize);int x=r[0],y=r[1],w=r[2],h=r[3];
   boolean chosen=i==selection&&topFocus<0,large=h>150;
   box(x,y,w,h,color(0xff1b293a),0);
   c.save();c.clipRect(x+2,y+2,x+w-2,y+h-2);
   int side=Math.max(w,h),left=x+(w-side)/2,top=y+(h-side)/2;
   if(g.art!=null){g.art.setBounds(left,top,left+side,top+side);g.art.draw(c);}
   p.setShader(null);p.setColor(0x40000000);c.drawRect(x,y,x+w,y+h,p);
   p.setColor(0xc0182019);c.drawRect(x,y+h-(large?91:58),x+w,y+h,p);c.restore();
   // Logos use contain-fit. Only decorative cover art is cropped to fill.
   if(g.icon!=null){int badge=large?82:40,lx=x+14,ly=y+12;box(lx-4,ly-4,badge+8,badge+8,0xb8101810,0);int iw=Math.max(1,g.icon.getIntrinsicWidth()),ih=Math.max(1,g.icon.getIntrinsicHeight());float fit=Math.min(badge/(float)iw,badge/(float)ih);int dw=Math.max(1,Math.round(iw*fit)),dh=Math.max(1,Math.round(ih*fit));int bx=lx+(badge-dw)/2,by=ly+(badge-dh)/2;g.icon.setBounds(bx,by,bx+dw,by+dh);g.icon.draw(c);}
   title(g.label,x+14,y+h-(large?66:34),w-28,large?21:16);
   if(large){box(x+14,y+h-31,112,23,chosen?accent:0xff263526,0);text(chosen?I18n.t("A  Spielen"):I18n.t("SPIEL"),x+25,y+h-14,12,chosen?0xff0b1b06:0xfff6f7f5,true);}
   box(x,y,w,h,chosen?accent:color(0xff293a4c),chosen?3:1);
  }
  if(games.isEmpty()){
   text(scanning?I18n.t("Spiele werden gesucht …"):I18n.t("Noch keine Spiele"),24,225,22,fg,true);
   text(I18n.t("Menü → Spiele auswählen"),24,258,16,muted,false);
   text(I18n.t("Dateien → APK installieren"),24,285,16,muted,false);
  }
  p.setColor(accent);p.setStrokeWidth(1);c.drawLine(24,418,616,418,p);
  text(I18n.t("A  Spielen"),24,447,14,accent,true);text(I18n.t("B  Zurück"),172,447,14,fg,false);text(I18n.t("START  Menü"),322,447,14,fg,false);
  text(I18n.t("X+Y halten · Apps schließen"),24,472,10,muted,false);
  text(games.size()+(games.size()==1?I18n.t(" SPIEL"):I18n.t(" SPIELE")),451,472,10,muted,true);text("GA36",579,472,10,muted,true);c.restore();
 }
}
