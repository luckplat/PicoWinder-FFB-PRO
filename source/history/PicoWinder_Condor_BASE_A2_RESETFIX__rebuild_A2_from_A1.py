#!/usr/bin/env python3
from pathlib import Path
import struct, hashlib

ROOT = Path(__file__).resolve().parent
A1 = ROOT / "PicoWinder_Condor_BASE_A1.raw"
RAW_OUT = ROOT / "PicoWinder_Condor_BASE_A2_RESETFIX.raw"
UF2_OUT = ROOT / "PicoWinder_Condor_BASE_A2_RESETFIX.uf2"

raw = bytearray(A1.read_bytes())
assert len(raw) == 0x6400
assert bytes(raw[0x60b8:0x60be]) == bytes.fromhex("01 20 00 f0 11 f9")
assert bytes(raw[0x6304:0x6318]) == b"\xff" * 20

raw[0x60b8:0x60be] = bytes.fromhex("00 f0 24 f9 00 bf")
raw[0x6304:0x6318] = bytes.fromhex(
    "00 b5 01 20 02 4b 98 47 06 20 01 4b 98 47 00 bd e1 62 00 10"
)
RAW_OUT.write_bytes(raw)

MAGIC0=0x0A324655; MAGIC1=0x9E5D5157; FLAGS=0x2000
FAMILY=0xE48BFF56; END=0x0AB16F30
num=len(raw)//256
uf2=bytearray()
for i in range(num):
    payload=raw[i*256:(i+1)*256]
    blk=bytearray(512)
    struct.pack_into("<IIIIIIII",blk,0,MAGIC0,MAGIC1,FLAGS,
                     0x10000000+i*256,256,i,num,FAMILY)
    blk[32:288]=payload
    struct.pack_into("<I",blk,508,END)
    uf2.extend(blk)
UF2_OUT.write_bytes(uf2)

print("RAW SHA256", hashlib.sha256(raw).hexdigest())
print("UF2 SHA256", hashlib.sha256(uf2).hexdigest())
