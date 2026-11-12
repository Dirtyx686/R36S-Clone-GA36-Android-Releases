> Current candidate: [64-GB+ dual boot](dual-boot/README.md). The older standalone images below failed the owner's boot test.

# Native boot, growth and import helpers

Actual C/header/include snapshots from the separate GA7/GA11 clean-image build.
The portable host build recipe uses Zig0.13.0 and creates ARM32 static helpers;
it does not install or write a card. e2fsck/resize2fs and image wrapping are
separate inputs. Hash constants and role UUIDs are device-layout guards, not
credentials. The clean standalone entry picks an OS at build time; it is not
a new dual-boot installer. Preboot code is retained for development reference.
Some compatibility fragments derive from upstream Android/kernel interfaces
and retain applicable terms; the source collection is not blanket relicensing.
