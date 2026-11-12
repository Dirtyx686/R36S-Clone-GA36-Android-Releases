package de.ga36.home;
import android.content.Context;import android.graphics.*;import android.view.*;
final class PowerScreen extends View {
 interface Actions{void choose(int index);}
 final Actions actions;final Paint p=new Paint(3);final RectF r=new RectF();final Typeface bold=Typeface.create("sans-serif",1),regular=Typeface.create("sans-serif",0);
 final Shader background=new LinearGradient(0,0,640,480,new int[]{0xff192643,0xff10172c,0xff090e1c},null,Shader.TileMode.CLAMP);
 static final int[] ACCENTS={0xffff8498,0xff6bdcff,0xff8aefbf},CARDS={0xff351e31,0xff152f4b,0xff17382f};
 int selected=PowerLayout.CANCEL,pressed=-1;float scale=1,ox,oy;Canvas c;
 PowerScreen(Context context,Actions a){super(context);actions=a;setFocusable(true);setFocusableInTouchMode(true);setContentDescription(I18n.t("GA36 Ausschaltmenü. Ausschalten, Neustart, Zurück. Steuerkreuz wählt, A bestätigt, B kehrt zurück."));}
 void text(String s,float x,float y,float size,int color,boolean strong){p.setShader(null);p.setStyle(Paint.Style.FILL);p.setColor(color);p.setTextSize(size);p.setTypeface(strong?bold:regular);c.drawText(s,x,y,p);}
 void box(float x,float y,float w,float h,int color,int stroke){p.setShader(null);p.setColor(color);p.setStyle(stroke==0?Paint.Style.FILL:Paint.Style.STROKE);p.setStrokeWidth(stroke);r.set(x,y,x+w,y+h);c.drawRoundRect(r,18,18,p);p.setStyle(Paint.Style.FILL);}
 protected void onDraw(Canvas canvas){paint(canvas,getWidth(),getHeight());}
 void paint(Canvas canvas,int width,int height){c=canvas;scale=Math.min(width/640f,height/480f);ox=(width-640*scale)/2;oy=(height-480*scale)/2;c.drawColor(0xff090e1c);c.save();c.translate(ox,oy);c.scale(scale,scale);p.setShader(background);c.drawRect(0,0,640,480,p);p.setShader(null);
  text(I18n.t("GA36  /  KONSOLE"),28,40,11,0xffa9bbdc,true);text("POWER",26,84,36,0xfff4f7ff,true);text(I18n.t("Was möchtest du tun?"),28,117,17,0xffbdcce3,false);
  for(int i=0;i<3;i++)box(28+i*28,134,20,4,ACCENTS[i],0);
  String[] titles={I18n.t("Ausschalten"),I18n.t("Neustart"),I18n.t("Zurück")},lines={I18n.t("Konsole ausschalten"),I18n.t("Android neu starten"),I18n.t("Weiterspielen")};
  for(int i=0;i<3;i++){float x=24+i*202;box(x,168,188,190,CARDS[i],0);box(x,168,188,190,selected==i?0xfff4f7ff:ACCENTS[i],selected==i?3:1);box(x+15,184,37,5,ACCENTS[i],0);
   float cx=x+94,cy=229;p.setColor(ACCENTS[i]);p.setStyle(Paint.Style.STROKE);p.setStrokeWidth(4);p.setStrokeCap(Paint.Cap.ROUND);
   if(i==0){r.set(cx-20,cy-20,cx+20,cy+20);c.drawArc(r,-50,280,false,p);c.drawLine(cx,cy-27,cx,cy-3,p);}
   if(i==1){r.set(cx-21,cy-21,cx+21,cy+21);c.drawArc(r,-45,270,false,p);float tx=cx-14.849242f,ty=cy-14.849242f;c.drawLine(tx,ty,tx-12,ty,p);c.drawLine(tx,ty,tx,ty+12,p);}
   if(i==2){c.drawLine(cx-22,cy,cx+22,cy,p);c.drawLine(cx-22,cy,cx-6,cy-16,p);c.drawLine(cx-22,cy,cx-6,cy+16,p);}
   p.setStrokeCap(Paint.Cap.BUTT);p.setStyle(Paint.Style.FILL);text(titles[i],x+16,284,19,0xfff4f7ff,true);text(lines[i],x+16,310,12,0xffc8d5e8,false);if(selected==i){box(x+15,326,158,18,ACCENTS[i],0);text(I18n.t("A  BESTÄTIGEN"),x+30,339,10,0xff091624,true);}
  }
  p.setColor(0xff33435e);p.setStrokeWidth(1);c.drawLine(28,400,612,400,p);text(I18n.t("Steuerkreuz  Auswahl"),28,433,12,0xffbdcce3,false);text(I18n.t("A  Bestätigen"),267,433,12,0xff8aefbf,true);text(I18n.t("B  Zurück"),499,433,12,0xffff8498,true);text(I18n.t("Kurz Power: Bildschirm sperren"),28,465,11,0xff9fb4d1,false);text("GA36 GAMING",520,465,10,0xff9fb4d1,true);c.restore();
 }
 public boolean onTouchEvent(MotionEvent e){int h=PowerLayout.hit((e.getX()-ox)/scale,(e.getY()-oy)/scale);if(e.getAction()==0){pressed=h;if(h>=0){selected=h;invalidate();}return true;}if(e.getAction()==1){if(h>=0&&h==pressed)actions.choose(h);pressed=-1;return true;}if(e.getAction()==3)pressed=-1;return true;}
}
