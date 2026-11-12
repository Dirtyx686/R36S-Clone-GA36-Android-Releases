package de.ga36.home;
final class GameLayout {
 static final int PAGE_SIZE=4,CARD_W=290,CARD_H=124;
 static int validSize(int n){return n==2||n==6?n:4;}
 static int columns(int size){return validSize(size)==6?3:2;}
 static int width(int size){return validSize(size)==6?189:290;}
 static int height(int size){return validSize(size)==2?258:124;}
 static int x(int i,int size){return 24+(i%columns(size))*(width(size)+12);}
 static int y(int i,int size){return 132+(i/columns(size))*134;}
 static int x(int i){return x(i,4);}static int y(int i){return y(i,4);}
 static int moveTop(int current,int key){current=Math.max(0,Math.min(3,current));if(key==21)return Math.max(0,current-1);if(key==22)return Math.min(3,current+1);if(key==19)return 3;if(key==20)return current==3?2:-1;return current;}
 static int hit(float x,float y,int count){return hit(x,y,count,4);}
 static int hit(float x,float y,int count,int size){if(y>=20&&y<=70&&x>=528&&x<=616)return -5;int[] xs={24,132,274},ws={96,130,170};if(y>=82&&y<=116)for(int i=0;i<3;i++)if(x>=xs[i]&&x<=xs[i]+ws[i])return -2-i;for(int i=0;i<count;i++)if(x>=x(i,size)&&x<=x(i,size)+width(size)&&y>=y(i,size)&&y<=y(i,size)+height(size))return i;return -1;}
 static int[] move(int selection,int page,int total,int key){return move(selection,page,total,key,4);}
 static int[] move(int selection,int page,int total,int key,int size){size=validSize(size);if(total<=0)return new int[]{0,0};page=Math.max(0,Math.min(page,(total-1)/size));int count=Math.min(size,total-page*size);selection=Math.max(0,Math.min(selection,count-1));int cols=columns(size),col=selection%cols,row=selection/cols,n=selection,np=page;
  if(key==21){if(col>0)n--;else if(page>0){np--;n=row*cols+cols-1;}}
  if(key==22){if(col+1<cols&&selection+1<count)n++;else if((page+1)*size<total){np++;n=row*cols;}}
  if(key==19&&row>0)n-=cols;
  if(key==20){if(selection+cols<count)n+=cols;else if((page+1)*size<total){np++;n=col;}}
  int nc=Math.min(size,total-np*size);return new int[]{Math.max(0,Math.min(n,nc-1)),np};
 }
}
