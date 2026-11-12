package de.ga36.home;
final class PowerLayout {
 static final int CANCEL=2;
 static int move(int current,int key){int step=(key==19||key==21)?-1:(key==20||key==22)?1:0;return Math.max(0,Math.min(2,current+step));}
 static int hit(float x,float y){if(y<168||y>358)return -1;for(int i=0;i<3;i++)if(x>=24+i*202&&x<=212+i*202)return i;return -1;}
}
