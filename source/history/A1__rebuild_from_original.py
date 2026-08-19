#!/usr/bin/env python3
"""Rebuild PicoWinder Condor BASE A1 starting from the STOCK PicoWinder raw image.

Inputs expected in same directory or edit paths below:
  original_picowinder.raw     stock PicoWinder raw flash image (0x5900 bytes)
  anti_freeze_transport.bin   0x300-byte transport module extracted from the
                              previously validated anti-freeze experiment
  ffb_output_engine_A1.bin    new OUTPUT-only FFB engine linked at 0x10005c00

This script deliberately does NOT use any TEST firmware as the base image.
"""
from pathlib import Path
import hashlib, struct

ROOT=Path(__file__).resolve().parent
ORIG=ROOT/'original_picowinder.raw'
TRANSPORT=ROOT/'anti_freeze_transport.bin'
ENGINE=ROOT/'ffb_output_engine_A1.bin'
RAW_OUT=ROOT/'PicoWinder_Condor_BASE_A1.raw'
UF2_OUT=ROOT/'PicoWinder_Condor_BASE_A1.uf2'

raw=bytearray(ORIG.read_bytes())
assert len(raw)==0x5900, f'Unexpected stock size: {len(raw):#x}'

# Minimal anti-freeze changes reconstructed ON TOP OF STOCK PicoWinder.
# These changes are transport/input-liveness only; they are not reused as the
# FFB semantics engine. They redirect the stock blocking MIDI output helpers to
# a non-blocking FIFO, skip HID output echo, add FIFO draining/PIO recovery,
# remove the stock light spring while retaining the trigger kick.
patches={
    0x0332: bytes.fromhex('04e0'),
    0x0934: bytes.fromhex('004b184701590010'),
    0x0974: bytes.fromhex('004b184771590010'),
    0x09a4: bytes.fromhex('004b184795590010'),
    0x09d8: bytes.fromhex('004b1847b9590010'),
    0x0a0c: bytes.fromhex('004b1847dd590010'),
    0x0d80: bytes.fromhex('04f044fe'),
    0x0e04: bytes.fromhex('00bf00bf'),  # disable example light spring only
    0x0e22: bytes.fromhex('04'),
    0x0e24: bytes.fromhex('6dfe'),
    0x0eb4: bytes.fromhex('5d5b'),
}
for off,data in patches.items():
    raw[off:off+len(data)]=data

transport=TRANSPORT.read_bytes()
assert len(transport)==0x300
raw.extend(transport)
assert len(raw)==0x5c00

# Redirect only HID-PID OUTPUT report handling. FEATURE CreateNewEffect,
# allocator, descriptor, input reports, PIO and TinyUSB behaviour remain stock.
stub=bytes.fromhex('00bf014b184700bfc046015c0010')
assert len(stub)==14
raw[0x037a:0x037a+len(stub)]=stub

engine=ENGINE.read_bytes()
raw.extend(engine)
while len(raw)%256: raw.append(0xff)
RAW_OUT.write_bytes(raw)

MAGIC0=0x0A324655; MAGIC1=0x9E5D5157; FLAGS=0x2000
FAMILY=0xE48BFF56; END=0x0AB16F30
num=len(raw)//256
uf2=bytearray()
for i in range(num):
    payload=raw[i*256:(i+1)*256]
    blk=bytearray(512)
    struct.pack_into('<IIIIIIII',blk,0,MAGIC0,MAGIC1,FLAGS,
                     0x10000000+i*256,256,i,num,FAMILY)
    blk[32:288]=payload
    struct.pack_into('<I',blk,508,END)
    uf2.extend(blk)
UF2_OUT.write_bytes(uf2)
print('RAW ',len(raw),hashlib.sha256(raw).hexdigest())
print('UF2 ',len(uf2),hashlib.sha256(uf2).hexdigest())
