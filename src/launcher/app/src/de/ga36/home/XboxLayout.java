package de.ga36.home;
/** Shared drawing, hit-testing and navigation geometry for the 640x480 console. */
final class XboxLayout {
 static final int NONE=-1, TOP_BASE=-10;
 static final int[] TOP_X={24,112,200,295,454},TOP_W={78,78,85,149,162};
 static int slots(boolean home,int size){return home?3:GameLayout.validSize(size);}
 static int[] rect(int i,boolean home,int size){
  if(home)return i==0?new int[]{24,170,374,230}:new int[]{410,170+(i-1)*121,206,109};
  int cols=GameLayout.columns(size),w=cols==3?189:290,h=size==2?230:109;
  return new int[]{24+(i%cols)*(w+12),170+(i/cols)*121,w,h};
 }
 static int top(int focus,int key){
  focus=Math.max(0,Math.min(4,focus));
  if(key==21)return Math.max(0,focus-1);
  if(key==22)return Math.min(4,focus+1);
  if(key==20)return -1;
  return focus;
 }
 static boolean firstRow(int selection,boolean home,int size){return home?selection!=2:selection<GameLayout.columns(size);}
 static int[] move(int selection,int page,int total,int key,boolean home,int size){
  int n=slots(home,size);if(total<=0)return new int[]{0,0};
  page=Math.max(0,Math.min(page,(total-1)/n));int count=Math.min(n,total-page*n);
  selection=Math.max(0,Math.min(selection,count-1));int next=selection,np=page;
  if(home){
   if(key==21){if(selection>0)next=0;else if(page>0){np--;next=1;}}
   if(key==22){if(selection==0&&count>1)next=1;else if((page+1)*n<total){np++;next=0;}}
   if(key==20){if(selection==0&&count>2)next=2;else if(selection==1&&count>2)next=2;else if((page+1)*n<total){np++;next=0;}}
   if(key==19&&selection==2)next=1;
  }else{
   int[] m=GameLayout.move(selection,page,total,key,size);next=m[0];np=m[1];
  }
  return new int[]{Math.max(0,Math.min(next,Math.min(n,total-np*n)-1)),np};
 }
 static int hit(float x,float y,int count,boolean home,int size){
  if(y>=82&&y<118)for(int i=0;i<5;i++)if(x>=TOP_X[i]&&x<TOP_X[i]+TOP_W[i])return TOP_BASE-i;
  for(int i=0;i<count;i++){int[] r=rect(i,home,size);if(x>=r[0]&&x<r[0]+r[2]&&y>=r[1]&&y<r[1]+r[3])return i;}
  return NONE;
 }
}
