# Contributing

Everyone is welcome to contribute, fork the repository and independently further
develop the project. Original GA36 code is MIT-licensed; third-party components
keep their own licenses. Contributions, device tests and bug reports are welcome.

Jeder darf mitmachen, das Repository forken und das Projekt eigenständig
weiterentwickeln. Dabei gelten die jeweils beiliegenden Lizenzen.

Target one confirmed board: GA36-MB V1.2-20260416 (runtime A33,512 MiB RAM).
State board revision, OS/channel, image checksum, flasher version and clear
reproduction steps in every report. Separate actual device tests from host,
static-analysis or emulator results. Reports for another board are exploratory.

For the Linux flasher, use Go1.27.4: cd tools/flasher/src && ./build.sh.
Tests write temporary regular files, not SD cards. Use only sacrificial cards
for an explicitly confirmed real write and verify the model/serial/size first.

For Android11, use the pinned upstream manifest and per-project patches/file
snapshot. Preserve upstream notices. Kernel config and build recipe are in
src/kernel; complete kernel sources belong to the matching source archive.
The launcher requires Android SDK/API30 and your own signing key. Never commit
private keys; a different key may require a fresh image, not an in-place update.

Prefer one measured fix per change. Explain the trigger, resulting behavior,
validation and remaining risks. Do not claim speed-ups without repeatable
measurements. No game binaries/data, raw user-data images, tokens, personal
serials or unreviewed firmware/artwork should be added to source commits.
