> Historical standalone candidate: owner reports no boot. Use the new [DualBoot](../DualBoot/README.md) candidate.

# GA7 — Android 7 Beta V1 / P39

Exact target: **GA36-MB V1.2-20260416**, R36S-style clone, runtime Allwinner A33,
512 MiB RAM. Not validated for other R36S boards or revisions.

Local candidate: `GA7-Clean-P39.img.gz`. The original local image is
preserved, with no user accounts or game APK/OBB packages. Its signed launcher
still contains the owner's game-logo/cover gimmick. The public launcher source
omits those assets; this image has not been rebuilt with that resource selection.
Do not present it as an artwork-clean public ROM.

K11 Linux3.4.39 #82, 256 MiB zRAM and a fully allocated 4 GiB SD swap file.
Physical RAM remains 512 MiB. Capacity: at least31,138,512,896 bytes (marketed
32GB or larger). The flasher adjusts the data partition and first boot grows
filesystems. Complete fresh-card boot/growth/import acceptance remains pending.

This is a standalone OS image; the local dual-boot development card is separate.
No image is committed to Git. IMAGE-INFOS.json pins the candidate package and
uncompressed image. Source/input provenance and missing vendor/boot evidence
are tracked in [known issues](../../KNOWN-ISSUES.md) and the parent overview.
