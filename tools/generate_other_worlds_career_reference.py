#!/usr/bin/env python3
"""Execute native MOAG promotion/award scanning over its unchanged cached theatre list."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_CS, UC_X86_REG_SS, UC_X86_REG_SP, UC_X86_REG_IP
from generate_native_reference import load, LOAD_ADDRESS

ROOT = Path(__file__).resolve().parents[1]
DS = 0x3b510
HEADER = 0x3600


def main():
    exe = (ROOT.parent/'corncob-reverse/installs/other_worlds_download/MOAG.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == '7dc6b71c77532e79ab5f05a48aa1b42bb57f0b975136102d42b133742b7b11be'
    stems = ('milkrun', 'flatland', 'quari', 'iceworld', 'midnight', 'tundra', 'newatkin', 'thin', 'exp', 'morning', 'dusk', 'wastelnd', 'whtsands', 'default', 'demo1', 'mtuniv')
    required = [((ROOT.parent / 'corncob-reverse/installs/other_worlds_download/DEF' / (name.upper() + '.DEF')).read_bytes()[10] & 128) != 0 for name in stems]
    rng = random.Random(0x4f544852)
    output = bytearray(b'CORNREV1'+struct.pack('<I',512))
    for i in range(512):
        pilot = bytearray(623)
        pilot[:22] = b'Career review'+bytes(9)
        struct.pack_into('<H',pilot,22,rng.choice((0,0,0,0,2,8)))
        struct.pack_into('<H',pilot,24,rng.randrange(1,100) if i<256 else rng.randrange(65536))
        struct.pack_into('<H',pilot,28,rng.choice((0,2,0x10,0x40,0x100,0x400,0x8000)))
        struct.pack_into('<H',pilot,32,rng.choice((0,0,1,3)))
        struct.pack_into('<H',pilot,100,rng.choice((0,1,4,5,7,32768,65535)))
        struct.pack_into('<H',pilot,102,rng.choice((0,0,0,0,0,1,3)))
        struct.pack_into('<H',pilot,104,rng.choice((0,26,27,28,32768,65535)))
        struct.pack_into('<i',pilot,158,rng.choice((0,10000,10001,20000,20001,50000,50001,100000,100001,-1)))
        struct.pack_into('<H',pilot,502,rng.choice((0,1,6,7,20,32768,65535)))
        pilot[598] = rng.randrange(7)
        for field in range(599,609):pilot[field] = rng.choice((0,0,0,1,10,255))
        now = 1_700_000_000+rng.randrange(1000000)
        struct.pack_into('<i',pilot,619,now+rng.choice((-1000,0,100000)))
        count = rng.randrange(21)
        active = rng.randrange(count) if count and i%4 else 128
        pilot[572],pilot[573] = count,active
        pilot[574:574+count] = bytes(range(1,count+1))
        losses = rng.choice((0,0,0,1,2))
        entries=[]
        for index in range(count):
            entries.append((rng.randrange(16),rng.choice((0,0,1,2,-1)),rng.choice((0,1,7,19,20,21,32767,-1)),rng.randrange(2)))
        # Explicitly cover complete campaigns and duplicate definitions as well as random partial careers.
        if i < 5:
            kinds = list(range(16)) if i < 2 else [0, 0, 0] if i == 2 else list(range(i - 1))
            entries = [(kind, 1 if i == 1 and kind == 0 else 0, 20, 0) for kind in kinds]
            count, active = len(entries), 0
            struct.pack_into('<H', pilot, 22, 0)
            struct.pack_into('<H', pilot, 102, 0)
            struct.pack_into('<H', pilot, 502, 20)
            pilot[598] = 0
            pilot[606] = 255
            pilot[572], pilot[573] = count, active
            pilot[574:574 + count] = bytes(range(1, count + 1))
        native = load(exe)
        for reg in (UC_X86_REG_DS,UC_X86_REG_ES,UC_X86_REG_SS):native.reg_write(reg,0x3b51)
        native.reg_write(UC_X86_REG_CS,0x3000)
        native.mem_write(DS+0xd000,bytes(pilot))
        native.mem_write(DS+0x1af4,struct.pack('<H',0xd000))
        native.mem_write(DS+0x7c75,struct.pack('<HH',16,0xd800))
        native.mem_write(DS+0xba9a,struct.pack('<H',losses))
        native.mem_write(DS+0xa9e,struct.pack('<H',0xd400 if count else 0))
        for kind in range(16):
            entry=bytearray(48);label=f'T{kind}'.encode()+b'\0';entry[:len(label)]=label
            struct.pack_into('<H',entry,44,required[kind])
            native.mem_write(DS+0xd800+kind*48,bytes(entry))
        for index,(kind,remaining,completed,credited) in enumerate(entries):
            entry=bytearray(40);label=f'T{kind}'.encode()+b'\0';entry[:len(label)]=label
            struct.pack_into('<5H',entry,30,remaining&65535,completed&65535,index+1,0xd400+(index+1)*40 if index+1<count else 0,credited)
            native.mem_write(DS+0xd400+index*40,bytes(entry))
        calls=[]
        def clock(uc,address,size,user):
            if address != LOAD_ADDRESS+0x150d:return
            sp=uc.reg_read(UC_X86_REG_SP)
            target=struct.unpack('<H',uc.mem_read(DS+sp+4,2))[0]
            uc.mem_write(DS+target,struct.pack('<i',now));calls.append('time')
            ip,cs=struct.unpack('<HH',uc.mem_read(DS+sp,4))
            uc.reg_write(UC_X86_REG_SP,(sp+4)&65535);uc.reg_write(UC_X86_REG_CS,cs);uc.reg_write(UC_X86_REG_IP,ip)
        native.hook_add(UC_HOOK_CODE,clock,begin=LOAD_ADDRESS+0x150d,end=LOAD_ADDRESS+0x150d)
        native.reg_write(UC_X86_REG_SP,0xefe0)
        native.mem_write(DS+0xefe0,struct.pack('<3H',0,0x2000,0xd000))
        native.emu_start(LOAD_ADDRESS+0x17ae4-HEADER,LOAD_ADDRESS,count=1000000)
        output+=pilot+struct.pack('<iHB',now,losses,count)
        for entry in entries:output+=struct.pack('<BhhB',*entry)
        output+=native.mem_read(DS+0xd000,623)
        for index in range(count):output+=native.mem_read(DS+0xd400+index*40+38,2)
    destination=ROOT/'tests/reference/other_worlds_career_review.bin'
    destination.write_bytes(output)
    destination.with_suffix('.json').write_text(json.dumps({
        'reference_sha256':hashlib.sha256(exe).hexdigest(),'fixture_sha256':hashlib.sha256(output).hexdigest(),
        'cases':512,'routine':'MOAG.EXE file 0x17ae4..0x17dfe',
        'stubs':['time(long*) supplies a fixed wall clock; every other helper runs unchanged'],
        'scope':'Complete promotion, medal and friendly-loss suspension decision, including native cached theatre lookup, strcmp, duplicate theatre definitions, current-index restrictions, per-theatre aircraft losses, signed comparisons and byte wrapping.'
    },indent=2)+'\n')
    print('Recorded 512 complete Other Worlds promotion, award and suspension decisions',flush=True)


if __name__ == '__main__':main()
