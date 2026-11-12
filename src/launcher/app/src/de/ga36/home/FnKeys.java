package de.ga36.home;
import java.util.HashSet;
/** Key-up ownership outlives FN release, so a consumed chord never leaks a release. */
final class FnKeys {
 static final int PASS=0,CONSUME=1,BRIGHTER=2,DARKER=3,HOME=4,PAUSE=5,RESTART=6,SAVE=7,LOAD=8,OPTIONS=9;
 private boolean fn,left,right;private final HashSet<Integer> consumed=new HashSet<Integer>();
 void reset(){fn=left=right=false;consumed.clear();}
 int key(int code,boolean down,int repeat){
  if(code==119){fn=down;return CONSUME;}
  if(code==106)left=down;if(code==107)right=down;
  if(!down)return consumed.remove(code)?CONSUME:PASS;
  if(consumed.contains(code)&&repeat>0)return fn&&code==19?BRIGHTER:fn&&code==20?DARKER:CONSUME;
  if(!fn)return PASS;
  int action=code==19?BRIGHTER:code==20?DARKER:(code==3||code==108)?HOME:(code==23||code==96)?PAUSE:(code==4||code==97)?RESTART:code==102?SAVE:code==103?LOAD:(code==82||code==98)?OPTIONS:PASS;
  if(code==106||code==107)action=left&&right?OPTIONS:CONSUME;
  if(action!=PASS){consumed.add(code);return repeat>0&&action!=BRIGHTER&&action!=DARKER?CONSUME:action;}
  return PASS;
 }
}
