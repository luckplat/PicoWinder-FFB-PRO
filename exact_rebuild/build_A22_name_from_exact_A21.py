#!/usr/bin/env python3
from pathlib import Path
import hashlib,struct

R=Path(__file__).resolve().parent
BASE=R/"BASE_A21_EXACT.uf2"
OUT=R/"PicoWinder_Condor_FINAL_A22_DEVICE_NAME.uf2"

assert hashlib.sha256(BASE.read_bytes()).hexdigest()=="839644adea2fe951ea626ea32b2c415b5dcacd6da6651da15b143994a01ba297"

M0=0x0A324655;M1=0x9E5D5157;FLAGS=0x2000
FAMILY=0xE48BFF56;END=0x0AB16F30;FLASH=0x10000000

u=BASE.read_bytes(); n=len(u)//512
raw=bytearray(b"\xff"*(n*256))
for i in range(n):
    b=u[i*512:(i+1)*512]
    m0,m1,fl,addr,size,bno,total,fam=struct.unpack_from("<IIIIIIII",b,0)
    assert (m0,m1,fl,size,bno,total,fam)==(M0,M1,FLAGS,256,i,n,FAMILY)
    raw[addr-FLASH:addr-FLASH+256]=b[32:288]

assert raw[0x4284:0x4284+11]==b"Picowinder\x00"
assert int.from_bytes(raw[0x5824:0x5828],"little")==0x10004284
assert raw[0x7048:0x7048+19]==b"\xff"*19

raw[0x5824:0x5828]=(0x10007048).to_bytes(4,"little")
raw[0x7048:0x7048+19]=b"Picowinder FFB PRO\x00"

out=bytearray()
for i in range(n):
    b=bytearray(512)
    struct.pack_into("<IIIIIIII",b,0,M0,M1,FLAGS,FLASH+i*256,256,i,n,FAMILY)
    b[32:288]=raw[i*256:(i+1)*256]
    struct.pack_into("<I",b,508,END)
    out+=b

OUT.write_bytes(out)
print("RAW SHA256",hashlib.sha256(raw).hexdigest())
print("UF2 SHA256",hashlib.sha256(out).hexdigest())
