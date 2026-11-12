# GA11 source/build handoff

The saved pinned repo manifest identifies863 upstream commits. Per-project
patches contain tracked changes; files/ also contains new source files. Apply
patches before copying modified files, or copy the final file snapshot directly;
do not apply the same modification twice. Preserve upstream notices.

The source-only device tree excludes signing keys, kernel Image, vendor shared
libraries and the old prebuilt launcher APK. Build the K11 source archive with
its saved configuration, validate its ABI/layout, and provide the resulting
Image in device/ga36/ga36/kernel/Image. Graphics Android.bp references retained
libraries that are not distributed by this source tree; these missing inputs
must be resolved before an end-to-end system build can succeed.

The actual development commands used the Lineage18.1 product:

```sh
export OUT_DIR=/path/to/separate-build-output
export GA36_ALLOW_EXPERIMENTAL_BUILD=true
source build/envsetup.sh
lunch lineage_ga36-userdebug
m -j4 Settings LatinIME libjni_latinime
```

Those commands rebuild the P39 settings/keyboard modules once dependencies are
available. They are not a demonstrated standalone recreation of the final ROM.
Audio recovery was reverted; the captured final audio source rebuild matched
the pre-P39 deployed HAL. Image wrapping/assembly also depends on retained
board boot and system inputs. Historical assembly recipes are collected locally,
not advertised as a turnkey public clean-ROM build. Do not write a device as a
side effect of a host source build.
