package de.ga36.home;
/** Delivery acknowledges the close request; focus independently proves a front window. */
final class PowerReveal {
 private boolean focused,closed,shown,cancelled;
 void focus(boolean value){focused=value;}
 void closed(){closed=true;}
 boolean take(){if(cancelled||shown||!focused||!closed)return false;shown=true;return true;}
 boolean shown(){return shown;}
 void cancel(){cancelled=true;}
}
