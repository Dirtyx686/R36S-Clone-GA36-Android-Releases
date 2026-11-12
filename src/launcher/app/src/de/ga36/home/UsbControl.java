package de.ga36.home;
final class UsbControl {
 static void select(Object manager,boolean files,boolean unlock){
  if(manager==null)throw new IllegalStateException(I18n.t("USB-Dienst fehlt"));
  try{
   // API24 has separate function and data-unlock methods; never report a request as connected.
   manager.getClass().getMethod("setCurrentFunction",String.class).invoke(manager,files?"mtp":"none");
   manager.getClass().getMethod("setUsbDataUnlocked",Boolean.TYPE).invoke(manager,files&&unlock);
  }catch(Exception e){throw new IllegalStateException(I18n.t("USB-Umschaltung fehlgeschlagen"),e);}
 }
 static String status(String state){return "CONFIGURED".equals(state)?I18n.t("PC verbunden · USB bereit"):"CONNECTED".equals(state)?I18n.t("PC erkannt · Verbindung wird eingerichtet"):I18n.t("Kein PC erkannt");}
}
