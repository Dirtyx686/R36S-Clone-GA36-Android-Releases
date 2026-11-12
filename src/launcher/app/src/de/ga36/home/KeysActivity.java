package de.ga36.home;
import android.os.Bundle;
public final class KeysActivity extends ConsolePanel {
 public void onCreate(Bundle b){super.onCreate(b);text(I18n.t("Tasten & Kurzbefehle"),25);text(I18n.t("X + Y gemeinsam 1,5 Sekunden halten\nHintergrund-Apps schließen"),17);text(I18n.t("FN + ↑ / ↓     Helligkeit\nFN + START     Zum Launcher\nFN + A             Spielmenü / Pause\nFN + B             Aktuelles Spiel neu starten\nFN + X / FN + L3 + R3     Spieloptionen"),17);text(I18n.t("Spielstände speichern, laden und Speicherplätze wechseln sind Emulator-Funktionen. Native Android-Spiele unterstützen diese Kombinationen nicht allgemein. Das Spielmenü pausiert über den normalen Android-Appwechsel; einzelne Spiele können im Hintergrund weiterlaufen."),13);button(I18n.t("Sticks prüfen"),new Runnable(){public void run(){StickProbe.show(KeysActivity.this);}});button(I18n.t("Zurück"),new Runnable(){public void run(){finish();}});}
}
