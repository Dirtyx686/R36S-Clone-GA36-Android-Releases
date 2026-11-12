# GA36 Linux Desktop Flasher 4.2 Compact — EXPERIMENT

Native Linux x86-64 desktop, English and German. Supports GA7/GA11 profiles
and the preserved 64-GB+ dual-boot layout. No Android image is included.

The compact `.ga36.gz` package requires this flasher or newer compatible
software; it is not a raw image for generic disk-writing tools. Keep its
IMAGE-INFOS.json beside the package and select Dual Boot. All data on the
selected target card is erased after explicit confirmation. The card must
hold at least 63,909,658,624 bytes. The smaller download does not lower that
capacity requirement.

Compressed-package and per-extent SHA-256 checks are followed by flushing and
readback of every written extent. Only proven-free blocks inside the two
prepared ext4 filesystems are omitted; filesystem metadata and all other
regions are written. The P39 compact package writes about 8.67 GB and reads
the same amount back. Duration depends on the card and reader. No physical
disk is opened in demo mode (`./GA36-Flasher-Linux-4.2-Compact --demo`).

Use X11 or XWayland, zenity for file dialogs, util-linux and polkit/pkexec for
privileged writing. APK/OBB queueing supports older GA36_APPS cards; it does
not support the new preserved dual-boot layout yet. USB/OTG and slot 2 remain
deferred. Windows binaries are not part of this release candidate.

## Offline source build

Use Go 1.27.4, CGO_ENABLED=0 and GOTOOLCHAIN=local. Run ./build.sh. Dependencies
are vendored, with their individual copyright/license files, plus LICENSES/.
The Linux build uses the nucular_shiny backend. The source contains other
platform files for development, but build.sh produces only the Linux binary.
Host tests use temporary regular files, not physical cards. The matching
compact package restore/readback test is documented separately.

MIT covers original flasher code only. Dependencies, fonts, Go runtime and any
external ROM retain their own terms. See LICENSE, LICENSES/, vendor/ and the
binary package's Drittlizenzen/. Publishing a flasher does not grant rights
to redistribute a ROM.

## Deutsch

Native Linux-Oberfläche mit DE/EN. Kein Android-Image enthalten. Das kompakte
GA36-Paket benötigt diesen passenden Flasher und weiterhin eine 64-GB-Karte
oder größer. Vor dem Schreiben wird das Löschen der ausgewählten Karte
bestätigt. Prüfsummen und vollständiges Rücklesen aller geschriebenen
Bereiche bleiben aktiv. Die neue Dual-Boot-Aufteilung unterstützt den alten
APK/OBB-Import noch nicht. Quellcode samt Abhängigkeiten und Lizenzen liegt
bei; ./build.sh baut nur Linux. Experimenteller Teststand.
