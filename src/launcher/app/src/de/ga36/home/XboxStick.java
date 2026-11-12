package de.ga36.home;
/** Hysteresis keeps stick drift from navigating; no global game input interception. */
final class XboxStick {
 static int direction(float x,float y,int held){
  if(Float.isNaN(x)||Float.isNaN(y)||Float.isInfinite(x)||Float.isInfinite(y))return 0;
  float threshold=held==0?.42f:.28f;
  if(Math.max(Math.abs(x),Math.abs(y))<threshold)return 0;
  return Math.abs(x)>=Math.abs(y)?(x<0?21:22):(y<0?19:20);
 }
}
