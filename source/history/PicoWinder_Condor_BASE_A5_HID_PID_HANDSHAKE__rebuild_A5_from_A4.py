#!/usr/bin/env python3
from pathlib import Path
import struct, hashlib

ROOT=Path(__file__).resolve().parent
SRC=ROOT/"PicoWinder_Condor_BASE_A4_NONBLOCKING_DEFINE.raw"
RAW_OUT=ROOT/"PicoWinder_Condor_BASE_A5_HID_PID_HANDSHAKE.raw"
UF2_OUT=ROOT/"PicoWinder_Condor_BASE_A5_HID_PID_HANDSHAKE.uf2"

raw=bytearray(SRC.read_bytes())
assert len(raw)==0x6400

# GET_REPORT dispatcher
assert raw[0x2da:0x2de]==bytes.fromhex("03 2a 01 d0")
raw[0x2da:0x2de]=bytes.fromhex("06 f0 27 f8")
g=bytes.fromhex(
"03 2a 0a d0 01 2a 06 d1 02 29 04 d1 12 20 28 70 00 20 68 70 02 24 "
"02 4b 18 47 02 4b 18 47 00 00 df 02 00 10 e3 02 00 10")
assert raw[0x632c:0x632c+len(g)]==b"\xff"*len(g)
raw[0x632c:0x632c+len(g)]=g

# PID Pool + BlockLoad Available
assert raw[0x2ea:0x2fc]==bytes.fromhex(
"27 23 2b 70 1d 3b ab 70 f5 33 6c 70 eb 70 04 24 f0 e7")
raw[0x2ea:0x2fc]=bytes.fromhex(
"ff 23 2b 70 6b 70 0a 23 ab 70 ff 23 eb 70 04 24 f0 e7")
assert raw[0x30c:0x314]==bytes.fromhex("00 f0 ce f9 ec 70 a8 70")
raw[0x30c:0x310]=bytes.fromhex("ff 20 ff 23")
raw[0x310:0x312]=bytes.fromhex("eb 70")

# Add PID State Input descriptor at relocated report descriptor.
oldlen=int.from_bytes(raw[0x43a1:0x43a3],"little")
assert oldlen==1044
old=bytes(raw[0x43ac:0x43ac+oldlen])
pid=bytes.fromhex(
"05 0f 09 92 a1 02 85 02 09 9f 09 a0 09 a4 09 a5 09 a6 "
"15 00 25 01 35 00 45 01 75 01 95 05 81 02 95 03 81 03 "
"09 94 15 00 25 01 35 00 45 01 75 01 95 01 81 02 "
"09 22 15 01 25 28 35 01 45 28 75 07 95 01 81 02 c0")
new=old[:-1]+pid+old[-1:]
assert len(new)==1113

raw.extend(b"\xff"*(0x7500-len(raw)))
raw[0x7000:0x7000+len(new)]=new
assert int.from_bytes(raw[0x570:0x574],"little")==0x100043ac
raw[0x570:0x574]=(0x10007000).to_bytes(4,"little")
raw[0x43a1:0x43a3]=len(new).to_bytes(2,"little")

# Reset + native autocentre OFF + PID State input notification
assert raw[0x60b8:0x60bc]==bytes.fromhex("00 f0 24 f9")
raw[0x60b8:0x60bc]=bytes.fromhex("00 f0 52 f9")
r=bytes.fromhex(
"10 b5 09 4c 01 20 a0 47 06 20 a0 47 82 b0 6a 46 12 20 10 70 "
"00 20 50 70 00 20 02 21 02 23 03 4c a0 47 02 b0 10 bd 00 00 "
"e1 62 00 10 7d 3a 00 10")
assert raw[0x6360:0x6360+len(r)]==b"\xff"*len(r)
raw[0x6360:0x6360+len(r)]=r

RAW_OUT.write_bytes(raw)

M0=0x0A324655; M1=0x9E5D5157; FLAGS=0x2000
FAMILY=0xE48BFF56; END=0x0AB16F30
n=len(raw)//256
uf2=bytearray()
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
