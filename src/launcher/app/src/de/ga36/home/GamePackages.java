package de.ga36.home;
final class GamePackages {
 static int coverKind(String name){
  if("com.skyrunner.game".equals(name))return 1;
  if("de.hackbook.diggis".equals(name))return 2;
  if("com.nightshift.r36".equals(name))return 3;
  if("de.simcitybrowser.app.r36s".equals(name))return 4;
  if("com.revc.game".equals(name))return 5;
  if("com.rockstargames.gtavc".equals(name))return 6;
  if("com.gameloft.android.ANMP.GloftA8HM".equals(name))return 7;
  if("com.ea.game.nfs14_row".equals(name))return 8;
  if("com.ea.games.nfs13_mod".equals(name))return 9;
  return 0;
 }
 static boolean isKnown(String name){return coverKind(name)!=0;}
 static String knownTitle(String name){
  switch(coverKind(name)){
   case 1:return "SkyRunner";case 2:return "Diggis Abenteuer";
   // The 2D and 3D releases share a package; keep the installed label.
   case 3:return null;case 4:return "SimCity Lite";case 5:return "reVC";
   case 6:return "GTA Vice City";case 7:return "Asphalt 8";
   case 8:return "NFS No Limits";case 9:return "On The Run";default:return null;
  }
 }
}
