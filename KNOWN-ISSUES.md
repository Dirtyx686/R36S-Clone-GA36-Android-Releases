# Experimental status / known issues

Latest owner report, 12 November 2026: GA7 and GA11 boot from the new clean
dual-boot test; GA7 audio works, GA11 audio works intermittently. The test card
was not independently read back to identify its exact image, so no broader
acceptance is inferred. Earlier sound/trailer reports do not close this issue.

- USB-to-PC/OTG and the second SD slot remain deferred.
- Sleep/wake has bugs and is unreliable; OS-specific scope is not yet confirmed.
- Settings, control navigation, game memory pressure and larger-card filesystem
  growth require further physical tests.
- APK/OBB imports via GA36_APPS do not support the preserved dual layout.
- Windows is a private test candidate; initial public binary scope is Linux.
- No game APKs/OBBs, accounts, saves or bundled game artwork in public sources.
- No full clean-room or fresh bit-identical ROM rebuild is demonstrated.
