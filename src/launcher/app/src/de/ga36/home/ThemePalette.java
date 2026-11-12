package de.ga36.home;
final class ThemePalette {
 static final int[] ACCENTS={0xff83de39,0xff6bdcff,0xffddbdff,0xffffa282};
 static int accent(int theme,int index){return theme==2?new int[]{0xff176c5c,0xff166280,0xff71459a,0xff8c4830}[index%4]:ACCENTS[index%4];}
 static int card(int theme,int index,boolean selected){if(theme==2)return new int[]{0xffe2f2ec,0xffe0edf8,0xffeee5f8,0xfff8e8df}[index%4];return selected?new int[]{0xff1c3d3a,0xff1c3854,0xff3a2b51,0xff4b302e}[index%4]:new int[]{0xff152e30,0xff182b44,0xff2b223f,0xff35272c}[index%4];}
 static final int[] BACKS={0xff101310,0xff20172f,0xfff4f7fb,0xff132544,0xff321725,0xff12362c,0xff342b17,0xff12303c};
 static final int[] ENDS={0xff080b08,0xff100c1b,0xffe6edf4,0xff080f23,0xff170e18,0xff091a17,0xff19160d,0xff081923};
 static final int[] PRIMARY={0xff83de39,0xffcf9eff,0xff176c5c,0xff83b8ff,0xffff97ab,0xffa8e58b,0xfff5cf75,0xff78dce9};
 static int color(int theme,int c){
  theme=theme<0||theme>=BACKS.length?0:theme;
  if(theme==0){if(c==0xff101a2b)return BACKS[0];if(c==0xff0a111d)return ENDS[0];if(c==0xff70ead0||c==0xff69e1c5)return PRIMARY[0];}
  if(theme>2){if(c==0xff101a2b)return BACKS[theme];if(c==0xff0a111d)return ENDS[theme];if(c==0xff70ead0||c==0xff69e1c5)return PRIMARY[theme];}

  if(theme==1){switch(c){
   case 0xff101a2b:return 0xff20172f;case 0xff0a111d:return 0xff100c1b;
   case 0xff1b293a:return 0xff2b203c;case 0xff203c44:return 0xff463052;
   case 0xff70ead0:case 0xff69e1c5:return 0xffcf9eff;
   case 0xff24434c:return 0xff47305a;case 0xff2a3c50:case 0xff293a4c:return 0xff524065;
   default:return c;
  }}
  if(theme==2){switch(c){
   case 0xff183f5f:return 0xffcce8f6;case 0xff3b2959:return 0xffe3d3f4;case 0xff6bdcff:return 0xff166280;case 0xffddbdff:return 0xff71459a;
   case 0xff101a2b:return 0xfff4f7fb;case 0xff0a111d:return 0xffe6edf4;
   case 0xff1b293a:return 0xffffffff;case 0xff203c44:return 0xffd6f1eb;
   case 0xff24434c:return 0xffd6f1eb;case 0xff283c50:return 0xffd9e4ee;
   case 0xff2a3c50:case 0xff293a4c:return 0xff8b9bab;
   case 0xffe9f0f8:case 0xfff1f6fa:case 0xffdce8f0:case 0xffd9e5ee:return 0xff172739;
   case 0xff93a7ba:case 0xffa5b7c8:case 0xff87a0b3:case 0xffafc7cd:case 0xff93a9bf:case 0xff9fb7ca:case 0xff688298:return 0xff475e72;
   case 0xff70ead0:case 0xff69e1c5:return 0xff117d6c;
   case 0xff092b28:case 0xff113732:case 0xff0d322d:return 0xffffffff;
   case 0xff80c6f0:return 0xff226993;case 0xff9db5ff:return 0xff5267a9;case 0xffd2a3ee:return 0xff8351a0;case 0xffeebd7e:return 0xff956321;case 0xff86ccb7:return 0xff2c7562;
   default:return c;
  }}return c;
 }
}
