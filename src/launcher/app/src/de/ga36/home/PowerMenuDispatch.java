package de.ga36.home;
/** One custom menu launch only after the ordered native-close broadcast returns. */
final class PowerMenuDispatch {
 private boolean pending;
 boolean begin(){if(pending)return false;pending=true;return true;}
 boolean complete(boolean interactive){if(!pending)return false;pending=false;return interactive;}
 void cancel(){pending=false;}
}
