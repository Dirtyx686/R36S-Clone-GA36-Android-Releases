#!/usr/bin/env python3
import os,sys,shutil
zig=os.environ.get("ZIG") or shutil.which("zig")
if not zig:raise SystemExit("Install Zig 0.13.0 and set ZIG")
args=[]
for arg in sys.argv[1:]:
 if arg.startswith("-Wp,-MD,"):args += ["-MD","-MF",arg.split(",",2)[2]]
 else:args.append(arg)
os.execv(zig,[zig,"cc","-target","x86_64-linux-musl","-static",*args])
