package de.ga36.home;
final class PowerMenuPolicy {
 static boolean accepts(String action,String reason){return "android.intent.action.CLOSE_SYSTEM_DIALOGS".equals(action)&&"globalactions".equals(reason);}
}
