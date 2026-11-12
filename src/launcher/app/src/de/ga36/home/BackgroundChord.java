package de.ga36.home;
/** Existing short key actions pass through. Only releases after the long chord are owned. */
final class BackgroundChord {
 static final long HOLD_MS=1500;boolean x,y,fired;long since=-1;
 void reset(){x=y=fired=false;since=-1;}
 boolean key(int scan,boolean down,long now){if(scan!=307&&scan!=308)return false;boolean consume=fired;if(scan==307)x=down;else y=down;if(x&&y){if(since<0&&!fired)since=now;}else since=-1;if(!x&&!y)reset();return consume;}
 long delay(long now){return !x||!y||fired||since<0?-1:Math.max(0,HOLD_MS-(now-since));}
 boolean fire(long now){if(delay(now)!=0)return false;fired=true;return true;}
}
