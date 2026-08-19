#!/usr/bin/env python3
from pathlib import Path
import hashlib,struct
R=Path(__file__).resolve().parent
raw=bytearray((R/"PicoWinder_Condor_BASE_A12_MESSAGE_PACING.raw").read_bytes())
assert hashlib.sha256(raw).hexdigest()=="5654e72c66c83e807135e87e4f5ae147c220a64bc3ac9f1cce56e7a30e776663"
for off,old in {
  0x0e0c:bytes.fromhex("fff77cfc"),
  0x0e46:bytes.fromhex("fff7adfd"),
  0x0e56:bytes.fromhex("fff7bffd"),
}.items():
    assert raw[off:off+4]==old
    raw[off:off+4]=bytes.fromhex("00bf00bf")
(R/"PicoWinder_Condor_BASE_A13_NO_KICK.raw").write_bytes(raw)
M0=0x0A324655;M1=0x9E5D5157;FLAGS=0x2000;FAMILY=0xE48BFF56;END=0x0AB16F30
n=len(raw)//256;u=bytearray()
for i in range(n):
    b=bytearray(512)
    struct.pack_into("<IIIIIIII",b,0,M0,M1,FLAGS,0x10000000+i*256,256,i,n,FAMILY)
    b[32:288]=raw[i*256:(i+1)*256]
    struct.pack_into("<I",b,508,END);u+=b
(R/"PicoWinder_Condor_BASE_A13_NO_KICK.uf2").write_bytes(u)
print("RAW",hashlib.sha256(raw).hexdigest())
print("UF2",hashlib.sha256(u).hexdigest())
