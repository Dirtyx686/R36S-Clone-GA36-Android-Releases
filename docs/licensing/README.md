# ROM provenance and license review — 12 November 2026

**EXPERIMENT. Preparation is complete for the findings below; ROM redistribution clearance is incomplete. Nothing was published.**

This review uses the actual English P39 dual-boot components, K11 #82 and the compact package derived from them. The ROM and physical cards were not changed. The compact package retains the same boot and Android binaries; a smaller download does not change their license obligations.

Updated evidence: [follow-up and source packages](FOLLOWUP-20261112.md). Amaze origin/license and BusyBox source/build lineage have now been collected; remaining limits are explicit.

## What is established

- Complete K11 kernel sources, configuration and build recipe are collected as `GA36-Kernel-K11-82-source.tar.gz`. Source ancestry and the reference kernel hash are recorded; no fresh bit-identical rebuild is claimed.
- GA11 integration has 863 pinned upstream projects, 13 collected locally changed projects and upstream notices. Native bootstrap and launcher source archives are collected; public launcher artwork excludes game logos/covers.
- e2fsprogs 1.45.4 sources and notices are collected. The two actual Android NOTICE files, Linux GPL text, Mali kernel header, musl, compiler-rt, Go and font notices are preserved under LICENSES/.
- The original GA7 Mali/EGL/GLES and gralloc files match exact blobs in the pinned `realthunder/a33_android_device_common` tree. Legacy audio HAL/dependencies and BusyBox match the pinned `CustomROMs/android_vendor_astar_chiphd` tree.
- GA11 `libMali.so` and `gralloc.ga36.so` are modified derivatives, not unmodified copies. The linker-dependency/SONAME changes and both hashes are recorded in BINARY-PROVENANCE.json.
- The shipped Mali kernel module identifies an earlier local K03 build. All 188 files in its r4p0 source directory match the corresponding directory preserved in the K11 source snapshot. This establishes source lineage, not a verified fresh module rebuild.

## Remaining ROM upload blockers

| Component actually shipped | Missing evidence or work |
| --- | --- |
| Mali-400 r4p0-00rel0 userspace: `libMali.so`, `libEGL_mali.so`, `libGLESv1_CM_mali.so`, `libGLESv2_mali.so`, and GA11 derivatives | Applicable package EULA or another explicit permission covering binary redistribution and modifications. No matching grant was found. Arm's newer Midgard r6p0 terms cannot be assumed to cover these 2014 Mali400 blobs. |
| `disp.ko`, `lcd.ko`, `gpio-sunxi.ko`, `udt_joystick.ko`; final build/config correspondence for `mali.ko` | Exact corresponding sources and build/install instructions for the shipped versions. All five carry GPL tags; ABI compatibility and similar BSP sources do not complete this evidence. The board joystick source was not located. |
| Retained 84-MiB prefix | Exact boot0/bootloader terms and corresponding vendor U-Boot sources/configuration. The distributed bytes also contain U-Boot `2017.09-g05bceb2-dirty` and a Linux `5.10.160` identifier. That older kernel is not the running K11; its inclusion still needs accounting. We do not infer that it executes. |
| BusyBox `1.20.0 (2012-08-22 21:36:24 CDT)` in bootstrap and GA7 | Official binary matches exactly; upstream BusyBox/uClibc source and Aboriginal hg1535 scripts/patches are now collected. The original expanded standalone configuration/fresh rebuild remains unverified. See follow-up. |
| Legacy GA7 audio HAL plus `libcodec_audio.so` and `libril_audio.so` | Per-file source/license or applicable binary grant. Exact origin is identified; the donor's Android NOTICE is not a blanket permission. |
| Remaining GA7 platform and third-party APKs | SDK donor terms and remaining component mapping. Amaze3.10 now matches the official APK and exact GPL source commit; dependency/source obligations remain. See follow-up. The 567-file focused inventory is not a full transitive software bill of materials. |

The exact file names, sizes, hashes, source matches and status are in BINARY-PROVENANCE.json/CSV. No negative inference about the original vendor's private agreements is made; those agreements were not available here. Absence of a located grant does not prove that no grant exists.

## Concrete publication scope

Linux flasher 4.2 (binary plus complete offline source and third-party notices), original project source, the K11 source archive and the GA11 integration/source archives can be prepared separately from a ROM download. Their own component terms still apply. This review does not grant any new rights or authorize publication.

Keep the existing ROM private while these binary-input gaps remain. To unblock a downloadable ROM: obtain the specific applicable grants/sources, or replace the affected components with source-built/licensed equivalents and repeat the device tests. A different option is a local builder that extracts required inputs from the owner's stock firmware; that builder has not been implemented or validated here. An “experimental” label or free download does not replace component permissions/source obligations.

## Current device report

Latest owner feedback: GA7 and GA11 boot; GA7 audio works; GA11 audio is currently silent. The report is not tied to a verified readback of the fresh test card, so no broader acceptance is inferred. USB/OTG and slot 2 remain deferred. The clean image contains no bundled games, saves or accounts.

## Quellen / Deutsch

Die Quellen und Lizenzbeilagen sind gesammelt und dateigenau zugeordnet. Für die Weitergabe der fertigen ROM fehlen weiterhin die passende Mali-Binärlizenz sowie einzelne exakte Herstellerquellen und weitere Komponenten-Nachweise. Diese Lücken lassen sich nicht durch eine eigene MIT-Lizenz schließen. Die funktionierende ROM wurde unverändert gelassen und nichts veröffentlicht.

Siehe RESEARCH-SOURCES.json für die Originalquellen und DONOR-TREE-CHECKS.json für die geprüften Revisionen. Die beiliegenden Lizenztexte gelten nur für ihre jeweiligen Komponenten.
