package de.ga36.home;
import android.os.Bundle;
/** Handles unsupported store links without changing the original game. */
public final class MarketLinkActivity extends ConsolePanel {
 public void onCreate(Bundle b){super.onCreate(b);text("Store-Link",25);text(I18n.t("Auf dieser Konsole ist kein App-Store eingerichtet."),17);button(I18n.t("Zurück zum Spiel"),new Runnable(){public void run(){finish();}}).requestFocus();}
}
