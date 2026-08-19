#!/usr/bin/env python3
from pathlib import Path
import hashlib,sys
p=Path(__file__).resolve().parents[1]/"firmware/PicoWinder_FFB_PRO_FINAL_A22.uf2"
want="49279876c95b18d4316cdd96e057593b9fdb733f064c0650adf3d5b9416b8103"
got=hashlib.sha256(p.read_bytes()).hexdigest()
print("UF2",got)
if got!=want:
    print("ERROR: hash mismatch",file=sys.stderr);sys.exit(1)
print("OK: exact FINAL A22")
