The Git folder contains configuration and build metadata. Extract the matching
GA36-Kernel-K11-82-source.tar.gz archive to obtain the complete source/ directory
before using build.sh. It is a later prepared source asset, not part of the
initial Linux-only flasher download.

# GA36 K11 / Linux 3.4.39 build #82

Complete source snapshot from the local tree used for K11 #82, plus the saved
.config, changed-source hashes and reference Image hash. Matching source hashes
and the current uncompressed kernel Image were checked during collection.
The source retains its upstream licenses. No vendor module binary or firmware
image is added by this source archive; retained-module correspondence remains
separately documented and is not supplied by this tree automatically.

Use the Android arm-eabi GCC 4.8 toolchain (not a modern aarch64 compiler).
The reference build used GCC source commit
325a57a9cceb570741de8b68c40f4a72136cc35d and binutils commit
9ddab0875454b0b5a8329a63c01b71880170575e, as recorded in the original toolchain
SOURCES file. Host tools used Zig 0.13.0 through a wrapper handling legacy
-Wp,-MD arguments. Set HOSTCC to the absolute path of hostcc-zig.py and ZIG to
the Zig executable. Source/configuration are provided; a fresh bit-identical
rebuild and physical boot of a rebuilt kernel have not been claimed here.

Run ./build.sh with CROSS_COMPILE, HOSTCC and JOBS set as needed. This builds
only on the host and never writes a card. Boot-image wrapping and board data
are separate. CONFIG_INITRAMFS_SOURCE is empty in the reference configuration.
