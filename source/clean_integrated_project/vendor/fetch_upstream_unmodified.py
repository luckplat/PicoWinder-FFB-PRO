#!/usr/bin/env python3
"""Fetch unchanged support files from the official upstream PicoWinder repo.

These files were never modified by the A2..A22 Condor work. The exact shipped
A22 UF2 is reproduced by exact_rebuild/, independently of this helper.
"""
from pathlib import Path
from urllib.request import urlopen

BASE="https://raw.githubusercontent.com/NolanNicholson/picowinder/main/"
FILES=[
    "ffb_handshake.pio",
    "read_joystick.pio",
    "hid_pid.h",
    "usb_descriptors.h",
]
# pico_sdk_import.cmake is the standard Pico SDK import helper. Prefer the
# upstream repository's copy for consistency.
FILES.append("pico_sdk_import.cmake")

root=Path(__file__).resolve().parent
for name in FILES:
    url=BASE+name
    print("fetch",url)
    data=urlopen(url,timeout=30).read()
    (root/name).write_bytes(data)
    print("  ->",root/name,len(data),"bytes")
