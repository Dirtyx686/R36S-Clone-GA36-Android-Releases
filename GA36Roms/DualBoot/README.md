# Clean English GA7 + GA11 dual boot

Exact GA36-MB V1.2-20260416 / Allwinner A33 / 512 MiB. Minimum card:
63,909,658,624 bytes (64 GB or larger). GA7 Beta V1 and GA11 Alpha V1 share
K11 #82 and retain the original boot prefix and raw offsets. Both data images
are fresh, initially 24 GiB, without games, accounts or saves. English is the
default for both ROMs, launcher, boot menu and boot animation. zRAM is 256 MiB;
shared SD swap is 4 GiB. Larger-card growth still needs physical testing.

D-pad selects and pauses the five-second GA7 timer. A or START boots the
selected system. Latest owner report: both systems boot; GA7 audio works, GA11 audio is intermittent. Sleep/wake has bugs.
The old GA36_APPS importer cannot target this preserved dual layout.
No ROM binary is in this source-selection repository. ROM downloads are provided as release assets; component source/provenance notes
are retained in the documentation.

Compact distribution uses GA36-extents-v1 and flasher 4.2 or compatible software.
The original 64-GB+ capacity requirement remains. See [the license review](../../docs/licensing/README.md)
for component origins, notices and recorded source/provenance limitations.

## Install on Linux

1. Download [the release](https://github.com/Dirtyx686/R36S-Clone-GA36-Android-Releases/releases/tag/dualboot-p39-experimental): `GA36-DualBoot-GA7-GA11-Compact-EN-P39.ga36.gz`,
   `IMAGE-INFOS.json`, `SHA256SUMS.txt` and `GA36-Flasher-Linux-4.2-Compact.tar.gz`.
2. Keep the compact ROM and `IMAGE-INFOS.json` together. Verify downloads with
   `sha256sum --ignore-missing -c SHA256SUMS.txt`.
3. Extract the flasher archive, run the included Linux desktop flasher, choose
   the compact ROM and the correct removable card (64 GB or larger). Writing
   overwrites the selected card. Allow its verification to finish before removal.
4. Insert the card into the console's system slot. Use the D-pad to select GA7
   or GA11; A or START confirms. GA7 starts after five seconds without input.

The `.ga36.gz` package is **not a raw image** for generic image writers. It uses
GA36-extents-v1 and requires GA36 Flasher 4.2 or compatible software. It retains
the 64-GB layout while writing and checking about 8.67 GB each. Do not use the
older standalone image candidates. Larger-card expansion still needs a device test.

## Deutsch

Experiment für das genannte GA36-Board, ab 64 GB. ROM, `IMAGE-INFOS.json`,
Prüfsummen und Linux-Flasher aus dem Release laden. ROM und JSON zusammen
ablegen, Flasher entpacken und starten. Die gewählte Karte wird überschrieben.
Prüfung vollständig abwarten. Beide Systeme starten laut Gerätetest; GA11-Ton
funktioniert nur zeitweise, Sleep/Aufwecken hat Bugs.
