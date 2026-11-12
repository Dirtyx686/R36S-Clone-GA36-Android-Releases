#!/usr/bin/env python3
from pathlib import Path
import hashlib,json,re,sys
root=Path(__file__).resolve().parents[1]
m=json.loads((root/'SOURCE-MANIFEST.json').read_text());expected={x['path']:x for x in m['files']};actual={}
for p in root.rglob('*'):
 rel=p.relative_to(root).as_posix()
 if rel=='.git' or rel.startswith('.git/'):continue
 if p.is_symlink():raise SystemExit('Symlink not included: '+rel)
 if not p.is_file() or rel=='SOURCE-MANIFEST.json':continue
 actual[rel]=p
if set(actual)!=set(expected):raise SystemExit('File set differs from source manifest')
for rel,p in actual.items():
 b=p.read_bytes();e=expected[rel]
 if len(b)!=e['bytes'] or hashlib.sha256(b).hexdigest()!=e['sha256']:raise SystemExit('Hash mismatch: '+rel)
 if rel not in m['reviewed_binary_assets']:
  if b'\0' in b:raise SystemExit('Unexpected binary: '+rel)
  text=b.decode('utf-8')
  if re.search(r'-----BEGIN [A-Z ]*PRIVATE KEY-----|github_pat_[A-Za-z0-9_]{20,}|gh[pousr]_[A-Za-z0-9]{20,}',text):raise SystemExit('Possible credential: '+rel)
 if rel.lower().endswith(('.apk','.obb','.img','.ext4','.so','.ko','.exe','.pk8','.pem','.jks','.keystore','.key')):raise SystemExit('Excluded artifact: '+rel)
 if '/res/' in rel and ('ga36_logo_' in rel or 'dashboard_art' in rel):raise SystemExit('Excluded game artwork: '+rel)
print('PASS: exact source file set/hashes; no ROM/game packages, signing keys, Windows executables or bundled game artwork.')
