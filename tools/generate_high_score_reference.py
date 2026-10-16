#!/usr/bin/env python3
"""Execute native high-score insertion with only its file load/save wrappers replaced."""
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_AX,UC_X86_REG_CS,UC_X86_REG_DS,UC_X86_REG_ES,UC_X86_REG_SS,UC_X86_REG_SP,UC_X86_REG_IP
from generate_native_reference import load,LOAD_ADDRESS

ROOT=Path(__file__).resolve().parents[1]
DS=0x38860
BASE=0x20000-0x3200
SHA='6d8c922d59a873073796b367a2a40993fcaa5e763ed2ca907a945981f0c5bc32'


def main():
    exe=(ROOT.parent/'corncob-reverse/installs/deluxe_dnk/MOAG.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest()==SHA
    rng=random.Random(0x48494748)
    output=bytearray(b'CORNHSC1'+struct.pack('<I',96))
    for case in range(96):
        pilot=bytearray(rng.randrange(256) for _ in range(623))
        label=['Training Mission','Atkinsville (easy)','White Sands Testing Ground','Dusk (medium)','Wasteland (medium)'][case%5]
        pilot[:6]=b'Pilot\0';pilot[572]=1;pilot[573]=128 if case%5==0 else 0;pilot[574]=7
        struct.pack_into('<H',pilot,24,[0,1,2,3,10,100,32767,32768,65535][case%9])
        struct.pack_into('<H',pilot,26,rng.randrange(10))
        struct.pack_into('<H',pilot,28,0x14)
        struct.pack_into('<i',pilot,158,rng.choice((-100,0,100,1000,10000)))
        struct.pack_into('<i',pilot,560,rng.choice((-100,0,100,1000,10000)))
        tables=[]
        for kind in range(3):
            size=180 if kind==0 else 162;table=bytearray()
            filled=10 if case%4>=2 else case%11
            for row in range(10):
                record=bytearray(rng.randrange(256) for _ in range(size))
                name=f'Other{row}'.encode()+b'\0'
                record[:len(name)]=name
                if row>=filled:record[0]=0
                if kind and case%3==0 and row==case%10:record[:6]=b'Pilot\0'
                struct.pack_into('<H',record,size-2,int(case%5==0))
                struct.pack_into('<i',record,size-6,(10-row)*100 if case%7 else struct.unpack_from('<i',pilot,158 if kind==0 else 560)[0])
                if kind:struct.pack_into('<H',record,22,rng.choice((1,3,10,100,32767,65535)))
                table+=record
            tables.append(table)
        native=load(exe)
        for register,value in ((UC_X86_REG_CS,0x2b5f),(UC_X86_REG_DS,0x3886),(UC_X86_REG_ES,0x3886),(UC_X86_REG_SS,0x3886)):
            native.reg_write(register,value)
        native.mem_write(DS+0xd000,bytes(pilot))
        native.mem_write(DS+0x1ba0,struct.pack('<H',0xd000))
        cached=bytearray(40);cached[:len(label)+1]=label.encode()+b'\0';struct.pack_into('<H',cached,34,7)
        native.mem_write(DS+0xd400,bytes(cached));native.mem_write(DS+0xb4a,struct.pack('<H',0xd400))
        for address,data in zip((0xa3f0,0x9d9c,0x9748),tables):native.mem_write(DS+address,bytes(data))
        def files(uc,address,size,user):
            if address not in (BASE+0xf61d,BASE+0xf6bd):return
            sp=uc.reg_read(UC_X86_REG_SP)
            ip,cs=struct.unpack('<2H',uc.mem_read(DS+sp,4))
            uc.reg_write(UC_X86_REG_AX,3);uc.reg_write(UC_X86_REG_SP,sp+4)
            uc.reg_write(UC_X86_REG_CS,cs);uc.reg_write(UC_X86_REG_IP,ip)
        native.hook_add(UC_HOOK_CODE,files)
        native.reg_write(UC_X86_REG_SP,0xf000)
        native.mem_write(DS+0xf000,struct.pack('<3H',0,0x2000,0xd000))
        native.emu_start(BASE+0xf269,LOAD_ADDRESS,count=1000000)
        assert native.reg_read(UC_X86_REG_CS)==0x2000 and native.reg_read(UC_X86_REG_IP)==0
        output+=bytes([len(label)])+label.encode()+pilot+b''.join(tables)
        for address,length in ((0xa3f0,1800),(0x9d9c,1620),(0x9748,1620)):output+=native.mem_read(DS+address,length)
    destination=ROOT/'tests/reference/high_scores.bin';destination.write_bytes(output)
    destination.with_suffix('.json').write_text(json.dumps({'reference_sha256':SHA,'fixture_sha256':hashlib.sha256(output).hexdigest(),'cases':96,'routines':['MOAG 0xf269..0xf61c insertion','0xf713 statistics comparator','0xf7d0 career-rate comparator'],'stubs':['0xf61d file loader: tables already supplied in native global buffers, returns descriptor 3','0xf6bd file writer: observes supplied in-memory tables without writing a file'],'scope':'All 5040 bytes after mission/career/fastest insertion, including empty names, occupied tables, score and kill-count ties, live/retired same-name rows, signed sortie counts, integer rates, original upward-only updates, and preserved string padding. Cached original theatre lookup is unchanged; includes the White Sands label and its original bounded overwrite into mission statistics.'},indent=2)+'\n')
    print('Recorded 96 complete original high-score insertion passes',flush=True)


if __name__=='__main__':main()
