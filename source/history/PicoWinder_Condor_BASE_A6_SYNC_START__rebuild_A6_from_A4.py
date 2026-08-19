#!/usr/bin/env python3
from pathlib import Path
import struct, hashlib

ROOT = Path(__file__).resolve().parent
BASE = ROOT / "PicoWinder_Condor_BASE_A4_NONBLOCKING_DEFINE.raw"
SYNC = ROOT / "a6_sync_start.bin"
RAW_OUT = ROOT / "PicoWinder_Condor_BASE_A6_SYNC_START.raw"
UF2_OUT = ROOT / "PicoWinder_Condor_BASE_A6_SYNC_START.uf2"

base = bytearray(BASE.read_bytes())
sync = SYNC.read_bytes()
assert len(base) == 0x6400
assert len(sync) == 140
assert base[0x5ec0:0x5ec4] == bytes.fromhex("00 f0 f8 f9")

def bl(addr,target):
    off=target-(addr+4)
    imm=off & ((1<<25)-1)
    S=(imm>>24)&1; I1=(imm>>23)&1; I2=(imm>>22)&1
    imm10=(imm>>12)&0x3ff; imm11=(imm>>1)&0x7ff
    J1=((~I1)&1)^S; J2=((~I2)&1)^S
    return struct.pack("<HH",
        0xF000|(S<<10)|imm10,
        0xD000|(J1<<13)|(J2<<11)|imm11)

assert bl(0x10005ec0,0x100062b4)==bytes.fromhex("00 f0 f8 f9")
base[0x5ec0:0x5ec4]=bl(0x10005ec0,0x10006400)

raw=base+bytearray(b"\xff"*0x100)
raw[0x6400:0x6400+len(sync)]=sync
RAW_OUT.write_bytes(raw)

M0=0x0A324655; M1=0x9E5D5157; FLAGS=0x2000
FAMILY=0xE48BFF56; END=0x0AB16F30
n=len(raw)//256; uf2=bytearray()
for i in range(n):
    p=raw[i*256:(i+1)*256]
    b=bytearray(512)
    struct.pack_into("<IIIIIIII",b,0,M0,M1,FLAGS,
                     0x10000000+i*256,256,i,n,FAMILY)
    b[32:288]=p
    struct.pack_into("<I",b,508,END)
    uf2.extend(b)
UF2_OUT.write_bytes(uf2)

print("RAW SHA256", hashlib.sha256(raw).hexdigest())
print("UF2 SHA256", hashlib.sha256(uf2).hexdigest())
