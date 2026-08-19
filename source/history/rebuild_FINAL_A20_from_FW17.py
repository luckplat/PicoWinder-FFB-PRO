#!/usr/bin/env python3
from pathlib import Path
import hashlib,struct,subprocess,re
R=Path(__file__).resolve().parent
raw=bytearray((R/"TESTED_FW17_BASE.raw").read_bytes())
assert hashlib.sha256(raw).hexdigest()=="2cde9f1ed917511c97967913763e88f1996661ecc7b849aacac022239570fe64"

clang="/usr/local/swift/usr/bin/clang"
lld="/usr/local/swift/usr/bin/ld.lld"
objcopy="/usr/local/swift/usr/bin/llvm-objcopy"
flags=["--target=arm-none-eabi","-mcpu=cortex-m0plus","-mthumb","-Os",
       "-ffreestanding","-fno-builtin","-fno-stack-protector"]

subprocess.run([clang,*flags,"-c",str(R/"FINAL_A20_input_filter.c"),
                "-o",str(R/"a20.o")],check=True)
subprocess.run([lld,"-T",str(R/"FINAL_A20_input_filter.ld"),"--gc-sections",
                str(R/"a20.o"),"-o",str(R/"a20.elf")],check=True)
subprocess.run([objcopy,"-O","binary",str(R/"a20.elf"),str(R/"a20.bin")],check=True)

nm=subprocess.check_output(["nm","-n",str(R/"a20.elf")],text=True)
m=re.search(r"^([0-9a-fA-F]+)\s+T\s+final_a20_hid_report$",nm,re.M)
assert m
target=int(m.group(1),16)&~1
assert target==0x10006E18

def BL(addr,target):
    off=target-(addr+4)
    imm=off & ((1<<25)-1)
    S=(imm>>24)&1;I1=(imm>>23)&1;I2=(imm>>22)&1
    imm10=(imm>>12)&0x3ff;imm11=(imm>>1)&0x7ff
    J1=((~I1)&1)^S;J2=((~I2)&1)^S
    return struct.pack("<HH",0xF000|(S<<10)|imm10,
                       0xD000|(J1<<13)|(J2<<11)|imm11)

raw[0x0E72:0x0E76]=BL(0x10000E72,target)
blob=(R/"a20.bin").read_bytes()
needed=0x6E00+len(blob)
flen=((needed+255)//256)*256
if len(raw)<flen: raw.extend(b"\xff"*(flen-len(raw)))
raw[0x6E00:flen]=b"\xff"*(flen-0x6E00)
raw[0x6E00:0x6E00+len(blob)]=blob

RAW=R/"PicoWinder_Condor_FINAL_A20_AXIS_REFINEMENT.raw"
RAW.write_bytes(raw)

M0=0x0A324655;M1=0x9E5D5157;FLAGS=0x2000;FAMILY=0xE48BFF56;END=0x0AB16F30
n=len(raw)//256
u=bytearray()
for i in range(n):
    b=bytearray(512)
    struct.pack_into("<IIIIIIII",b,0,M0,M1,FLAGS,0x10000000+i*256,256,i,n,FAMILY)
    b[32:288]=raw[i*256:(i+1)*256]
    struct.pack_into("<I",b,508,END)
    u+=b
(R/"PicoWinder_Condor_FINAL_A20_AXIS_REFINEMENT.uf2").write_bytes(u)
print("RAW",hashlib.sha256(raw).hexdigest())
print("UF2",hashlib.sha256(u).hexdigest())
