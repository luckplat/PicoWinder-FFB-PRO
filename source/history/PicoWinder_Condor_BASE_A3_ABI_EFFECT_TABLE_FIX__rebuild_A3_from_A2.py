#!/usr/bin/env python3
from pathlib import Path
import struct, hashlib

ROOT=Path(__file__).resolve().parent
SRC=ROOT/"PicoWinder_Condor_BASE_A2_RESETFIX.raw"
RAW_OUT=ROOT/"PicoWinder_Condor_BASE_A3_ABI_EFFECT_TABLE_FIX.raw"
UF2_OUT=ROOT/"PicoWinder_Condor_BASE_A3_ABI_EFFECT_TABLE_FIX.uf2"

raw=bytearray(SRC.read_bytes())
assert len(raw)==0x6400
patches={
0x5d72:("ab00","2b00"),
0x5d76:("1c59","1c5d"),
0x5e0a:("a000","2000"),
0x5e0e:("4250","4254"),
0x5ffc:("a800","2800"),
0x6000:("4258","425c"),
0x60d0:("1360","1370"),
0x60d4:("121d","0132"),
}
for off,(b,a) in patches.items():
    b=bytes.fromhex(b); a=bytes.fromhex(a)
    assert raw[off:off+len(b)]==b, (hex(off),raw[off:off+len(b)].hex(),b.hex())
    raw[off:off+len(a)]=a
RAW_OUT.write_bytes(raw)

M0=0x0A324655; M1=0x9E5D5157; FLAGS=0x2000
FAMILY=0xE48BFF56; END=0x0AB16F30
n=len(raw)//256; uf2=bytearray()
for i in range(n):
    p=raw[i*256:(i+1)*256]
    blk=bytearray(512)
    struct.pack_into("<IIIIIIII",blk,0,M0,M1,FLAGS,0x10000000+i*256,256,i,n,FAMILY)
    blk[32:288]=p
    struct.pack_into("<I",blk,508,END)
    uf2.extend(blk)
UF2_OUT.write_bytes(uf2)

print("RAW SHA256",hashlib.sha256(raw).hexdigest())
print("UF2 SHA256",hashlib.sha256(uf2).hexdigest())
