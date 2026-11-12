# GA36 launcher source — public resource selection

P39 launcher102 Java/XML sources, German/English strings, console navigation,
themes, 2/4/6 library layouts and the neutral GA36 app icon. All fixed game-logo
PNG resources and cover JPEG artwork are deliberately omitted at the owner's
request. The private launcher and existing ROM image bytes remain unchanged.
ARTWORK-OMITTED.json lists names and hashes without distributing those images.

GameLogos/DashboardArt lookups already handle absent optional resources. The
public build therefore falls back to neutral/generated icons and installed app
icons; no copyrighted game artwork needs to be bundled. This source selection
is not a byte-identical source/resources set for the private signed APK.
A new launcher build, ROM update and device test are required before claiming
that a public ROM contains the resource-clean variant.

Build against Android API30 with minSdk24, Java8-compatible bytecode. A signing
key/certificate must be supplied by the builder. The owner's signing key is
not included. Building your own key does not replace the private platform key
or make a privileged APK update automatically compatible with existing images.
Original project code is MIT; Android API and imported code keep their terms.

The public launcher Java/Dex/unsigned-APK packaging check now passes with zero
embedded game-logo/cover files and only the neutral GA36 icon. The private signed
APK and ROMs are unchanged; signed-image/device acceptance remains separate.
