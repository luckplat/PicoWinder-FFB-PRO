#!/usr/bin/env python3
from pathlib import Path
import hashlib, struct
R=Path(__file__).resolve().parent
raw=bytearray((R/"PicoWinder_Condor_BASE_A13_NO_KICK.raw").read_bytes())
assert hashlib.sha256(raw).hexdigest()=="e9345232307880ce4aebaf34343a52056cf84c2217470116902c827fdfc1af75"
assert raw[0x6018:0x601a]==bytes.fromhex("4208")
raw[0x6018:0x601a]=bytes.fromhex("0246")
(R/"PicoWinder_Condor_BASE_A14_PERIODIC_GAIN_FIX.raw").write_bytes(raw)
M0=0x0A324655;M1=0x9E5D5157;FLAGS=0x2000;FAMILY=0xE48BFF56;END=0x0AB16F30
n=len(raw)//256;u=bytearray()
for i in range(n):
    b=bytearray(512)
    struct.pack_into("<IIIIIIII",b,0,M0,M1,FLAGS,0x10000000+i*256,256,i,n,FAMILY)
    b[32:288]=raw[i*256:(i+1)*256]
    struct.pack_into("<I",b,508,END)
    u+=b
(R/"PicoWinder_Condor_BASE_A14_PERIODIC_GAIN_FIX.uf2").write_bytes(u)
print("RAW",hashlib.sha256(raw).hexdigest())
print("UF2",hashlib.sha256(u).hexdigest())
