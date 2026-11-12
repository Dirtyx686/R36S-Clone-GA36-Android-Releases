package de.ga36.home;
import android.os.Bundle;import android.graphics.Color;import android.view.*;import android.widget.*;
abstract class ConsolePanel extends FullscreenActivity {
 LinearLayout panel;
 protected void onCreate(Bundle b){super.onCreate(b);panel=new LinearLayout(this);panel.setOrientation(1);panel.setPadding(24,12,24,12);panel.setBackgroundColor(0xff101310);ScrollView scroll=new ScrollView(this);scroll.setFillViewport(true);scroll.addView(panel);setContentView(scroll);}
 TextView text(String label,int size){TextView t=new TextView(this);t.setText(label);t.setTextColor(0xffe9f0f8);t.setTextSize(size);t.setPadding(0,4,0,4);panel.addView(t);return t;}
 Button button(String label,final Runnable action){Button b=new Button(this);b.setText(label);b.setAllCaps(false);b.setOnClickListener(new View.OnClickListener(){public void onClick(View v){action.run();}});panel.addView(b,new LinearLayout.LayoutParams(-1,48));return b;}
 public boolean dispatchKeyEvent(KeyEvent e){int k=e.getKeyCode();if(k==96)return super.dispatchKeyEvent(new KeyEvent(e.getDownTime(),e.getEventTime(),e.getAction(),23,e.getRepeatCount()));if(k==97){if(e.getAction()==0&&e.getRepeatCount()==0)finish();return true;}return super.dispatchKeyEvent(e);}
}
