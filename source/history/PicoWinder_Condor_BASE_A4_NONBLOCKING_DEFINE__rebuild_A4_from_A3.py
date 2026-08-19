#!/usr/bin/env python3
from pathlib import Path
import struct, hashlib

ROOT=Path(__file__).resolve().parent
SRC=ROOT/"PicoWinder_Condor_BASE_A3_ABI_EFFECT_TABLE_FIX.raw"
RAW_OUT=ROOT/"PicoWinder_Condor_BASE_A4_NONBLOCKING_DEFINE.raw"
UF2_OUT=ROOT/"PicoWinder_Condor_BASE_A4_NONBLOCKING_DEFINE.uf2"

raw=bytearray(SRC.read_bytes())
assert len(raw)==0x6400
expected=bytes.fromhex(
"20 22 a3 69 1a 42 fc d1 0b 78 01 36 23 60 01 31 86 42 f6 d3"
)
assert raw[0x7f0:0x804]==expected

raw[0x7f0:0x7f8]=bytes.fromhex("00 4b 18 47 19 63 00 10")
tramp=bytes.fromhex(
"01 46 38 46 01 4b 98 47 01 4b 18 47 31 59 00 10 05 08 00 10"
)
assert raw[0x6318:0x6318+len(tramp)]==b"\xff"*len(tramp)
raw[0x6318:0x6318+len(tramp)]=tramp
RAW_OUT.write_bytes(raw)

M0=0x0A324655; M1=0x9E5D5157; FLAGS=0x2000
FAMILY=0xE48BFF56; END=0x0AB16F30
n=len(raw)//256; uf2=bytearray()
for i in range(n):
    p=raw[i*256:(i+1)*256]
    blk=bytearray(512)
    struct.pack_into("<IIIIIIII",blk,0,M0,M1,FLAGS,
                     0x10000000+i*256,256,i,n,FAMILY)
    blk[32:288]=p
    struct.pack_into("<I",blk,508,END)
    uf2.extend(blk)
UF2_OUT.write_bytes(uf2)
print("RAW SHA256",hashlib.sha256(raw).hexdigest())
print("UF2 SHA256",hashlib.sha256(uf2).hexdigest())
