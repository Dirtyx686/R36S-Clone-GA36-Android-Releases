package de.ga36.home;
final class CatalogState {
 private boolean dirty=true,loading=false;
 void invalidate(){dirty=true;}
 boolean begin(){if(loading||!dirty)return false;loading=true;dirty=false;return true;}
 void complete(boolean success){loading=false;if(!success)dirty=true;}
 boolean isLoading(){return loading;}
 boolean isDirty(){return dirty;}
}
