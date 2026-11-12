# Current 64-GB+ dual-boot native source

P39/K11 #82 exact board. Entry point: dual-init.c, including runtime-init.c.
Keeps the original raw offsets and Binder7/Binder8 same-kernel selection.
Both fresh userdata images begin at 24 GiB. Larger raw cards grow the outer
pool before mounting; dual-images.c splits the available budget equally.
The boot menu defaults to GA7 after five seconds. D-pad pauses the timer;
A/START confirms. make-boot-assets.py generates English black-background logos.

Build static ARM32 helpers with Zig 0.13 and ./build.sh. Run --selftest under
ARM emulation. These tests do not exercise the actual framebuffer or storage.
Packaging still needs the exact immutable K11 kernel, original boot prefix,
root archives matching v1-config.h and modules with their source/provenance.
Never substitute a standard DOS MBR for the original Allwinner prefix.
The older native directory is a historical standalone-image recipe; those
images failed the owner's boot test. The new clean dual image is pending a
physical acceptance test. No private signing key, ROM or game is included here.
