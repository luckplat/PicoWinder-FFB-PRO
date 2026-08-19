#!/usr/bin/env python3
from pathlib import Path
import hashlib,struct,subprocess,re
R=Path(__file__).resolve().parent
BASE=R/"PicoWinder_Condor_BASE_A15_SPRING_CURVE.raw"
SRC=R/"a16_axis_hysteresis.c"
LD=R/"a16_link.ld"
OBJ=R/"a16_axis_hysteresis.o"
ELF=R/"a16_axis_hysteresis.elf"
BIN=R/"a16_axis_hysteresis.bin"

raw=bytearray(BASE.read_bytes())
assert hashlib.sha256(raw).hexdigest()=="519009b6cc86d505bb88c71b1e4446bcb04c4e444904fbde51541ffbde989a75"

clang="/usr/local/swift/usr/bin/clang"
lld="/usr/local/swift/usr/bin/ld.lld"
objcopy="/usr/local/swift/usr/bin/llvm-objcopy"

subprocess.run([clang,"--target=arm-none-eabi","-mcpu=cortex-m0plus","-mthumb",
 "-Os","-ffreestanding","-fno-builtin","-fno-stack-protector",
 "-c",str(SRC),"-o",str(OBJ)],check=True)
subprocess.run([lld,"-T",str(LD),"--gc-sections",str(OBJ),"-o",str(ELF)],check=True)
subprocess.run([objcopy,"-O","binary",str(ELF),str(BIN)],check=True)

nm=subprocess.check_output(["nm","-n",str(ELF)],text=True)
m=re.search(r"^([0-9a-fA-F]+)\s+T\s+a16_hid_report$",nm,re.M)
assert m
target=int(m.group(1),16)&~1

def bl(addr,target):
    off=target-(addr+4)
    imm=off & ((1<<25)-1)
    S=(imm>>24)&1; I1=(imm>>23)&1; I2=(imm>>22)&1
    imm10=(imm>>12)&0x3ff; imm11=(imm>>1)&0x7ff
    J1=((~I1)&1)^S; J2=((~I2)&1)^S
    return struct.pack("<HH",0xF000|(S<<10)|imm10,
                       0xD000|(J1<<13)|(J2<<11)|imm11)

assert raw[0xe72:0xe76]==bl(0x10000e72,0x10003a7c)
raw[0xe72:0xe76]=bl(0x10000e72,target)

blob=BIN.read_bytes()
section=0x10006D00
needed=(section-0x10000000)+len(blob)
final=((needed+0xff)//0x100)*0x100
raw.extend(b"\xff"*(final-len(raw)))
off=section-0x10000000
raw[off:off+len(blob)]=blob

RAW=R/"PicoWinder_Condor_BASE_A16_AXIS_HYSTERESIS.raw"
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
(R/"PicoWinder_Condor_BASE_A16_AXIS_HYSTERESIS.uf2").write_bytes(u)
print("RAW",hashlib.sha256(raw).hexdigest())
print("UF2",hashlib.sha256(u).hexdigest())
