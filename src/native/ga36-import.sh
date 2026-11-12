#!/system/bin/sh
# GA7/GA11 card-reader import. Runs once after boot as Android's shell user.
PATH=/system/bin:/system/xbin
export PATH
box=/data/local/tmp/ga36-inbox
[ -d "$box/APKS" ] && [ -d "$box/OBB" ] || exit 0
hashfile() { /data/local/tmp/ga36-hash "$1"; }
validpkg() { case "$1" in ''|*[!A-Za-z0-9_.]*|.*|*..*|*.) return 1;; esac; case "$1" in [A-Za-z]*.*) return 0;; *) return 1;; esac; }
started=0
logstart() { [ "$started" = 1 ] && return; started=1; printf 'GA36 import / %s\n' "$(date)" > "$box/INSTALL.TXT"; }
log() { printf '%s\n' "$*" >> "$box/INSTALL.TXT"; }
for job in "$box"/OBB/*; do
 [ -d "$job" ] && [ -f "$job/PACKAGE.TXT" ] && [ -f "$job/SHA256.TXT" ] || continue
 logstart
 pkg=$(cat "$job/PACKAGE.TXT")
 if ! validpkg "$pkg"; then log 'ERROR: Invalid OBB package'; continue; fi
 target="/sdcard/Android/obb/$pkg"
 if ! mkdir -p "$target"; then log "ERROR: Cannot create $target"; continue; fi
 ok=1; count=0
 while read -r sha name extra; do
  count=$((count+1))
  case "$name" in ''|*[!A-Za-z0-9_.]*) ok=0; break;; esac
  case "$name" in main.*."$pkg".obb|patch.*."$pkg".obb) ;; *) ok=0; break;; esac
  [ -z "$extra" ] && [ "${#sha}" = 64 ] && [ -f "$job/$name" ] && [ ! -L "$job/$name" ] || { ok=0; break; }
  got=$(hashfile "$job/$name")
  [ "$got" = "$sha" ] || { ok=0; break; }
  if ! cp "$job/$name" "$target/.$name.ga36-part"; then ok=0; break; fi
  got=$(hashfile "$target/.$name.ga36-part")
  if [ "$got" != "$sha" ] || ! mv "$target/.$name.ga36-part" "$target/$name"; then rm -f "$target/.$name.ga36-part"; ok=0; break; fi
 done < "$job/SHA256.TXT"
 if [ "$ok" = 1 ] && [ "$count" -gt 0 ]; then sync; rm -rf "$job"; log "OK OBB $pkg"; else log "ERROR OBB $pkg; sources retained"; fi
 done
for apk in "$box"/APKS/*.apk; do
 [ -f "$apk" ] && [ ! -L "$apk" ] && [ -f "$apk.pkg" ] || continue
 logstart
 pkg=$(cat "$apk.pkg");base=${apk##*/};wanted=${base%%-*}
 if ! validpkg "$pkg" || [ "${#wanted}" != 64 ]; then log 'ERROR: Invalid APK job'; continue; fi
 got=$(hashfile "$apk")
 [ "$got" = "$wanted" ] || { log "ERROR APK hash $pkg"; continue; }
 stage="/data/local/tmp/ga36-install-$wanted.apk"
 if ! cp "$apk" "$stage" || [ "$(hashfile "$stage")" != "$wanted" ]; then rm -f "$stage"; log "ERROR APK copy $pkg"; continue; fi
 result=$(pm install -r -g --user 0 "$stage" 2>&1);rc=$?;rm -f "$stage"
 case "$result" in *Success*) if [ "$rc" = 0 ]; then sync; rm -f "$apk" "$apk.pkg"; log "OK APK $pkg"; else log "ERROR APK $pkg: $result"; fi;; *) log "ERROR APK $pkg: $result";; esac
done
[ "$started" = 0 ] || sync
exit 0
