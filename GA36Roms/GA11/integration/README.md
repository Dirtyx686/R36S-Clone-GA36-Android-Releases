# GA36 Android 11 / P39 integration source snapshot

Pinned upstream repo manifest, actual source changes and source-only GA36
device tree. Apply per-project patches to the pinned upstream checkout, then
copy `files/` over that checkout to include new source files. Do not re-apply
a patch after copying its already-modified source file. projects.json records
what was collected and any omitted binary/private/generated changes.

No signing private keys, donor libraries, APKs, kernel image, game data or
stock ROM are included. Public configuration contains references to locally
retained components; those inputs must be provided separately with appropriate
provenance. The vendor/device tree reflects the current checkout, not a claim
that all build products were used in the shipped clean image. P39 reverted
the experimental audio changes; consult the audio-rollback record before
building an image. A complete clean-room rebuild of the final GA11 image has
not been demonstrated by this collection.

Framework changes retain upstream licenses; upstream notices are included.

The public Git selection omits two historical host-path-only preparation JSON
reports (old K04 kernel/graphics preparation). They are not current P39 evidence
or runtime configuration. The kernel Image and vendor libraries must be supplied
separately; use the K11 #82 reference, not the old K04 metadata.
