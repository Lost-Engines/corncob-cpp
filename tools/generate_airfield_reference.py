#!/usr/bin/env python3
"""Execute unchanged MOAG survey descriptions, mission grouping and Borland sorting.

Requires Unicorn and the sibling research tree; normal builds use the fixture.
Only file output and allocation are supplied by the harness. Original string,
integer arithmetic, grouping, comparator and qsort code execute unchanged.
"""
from pathlib import Path
import hashlib, json, random, struct, sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT.parent/'corncob-reverse/tools'))
from moag_survey_native import Survey,exe

def main():
    assert hashlib.sha256(exe).hexdigest()=='6d8c922d59a873073796b367a2a40993fcaa5e763ed2ca907a945981f0c5bc32'
    rng=random.Random(0x41495246);s=Survey();cases=[]
    for kind in range(61):
        for quality in range(8):
            for extra in (0,0x100,0x800,0x900):
                centre=(rng.randrange(-3_000_000,3_000_000),rng.randrange(-3_000_000,3_000_000))
                record=(rng.randrange(-5_000_000,5_000_000),rng.randrange(-5_000_000,5_000_000),rng.randrange(-10000,300000),kind,1|(quality<<12)|extra,rng.randrange(1,16))
                cases.append((record,centre,s.render(record,centre)))
    output=bytearray(b'CORNINT1'+struct.pack('<II',len(cases),96))
    for record,centre,text in cases:
        encoded=text.encode('cp437');output+=struct.pack('<iiiHHBiiH',*record,*centre,len(encoded))+encoded
    for trial in range(96):
        count=rng.randrange(1,64)
        records=[]
        for i in range(count):
            kind=rng.choice((0,0,1,2,2,6,7,7,11,12,13,23,24,27,33,40,41,43,49,50,51,53,58))
            records.append((rng.randrange(-20,21)*100000,rng.randrange(-20,21)*100000,rng.randrange(0,10000),kind,1|(rng.randrange(8)<<12)|rng.choice((0,0x200,0x800)),0))
        s.uc.mem_write(0x60000,b''.join(struct.pack('<iiiHHB',*record) for record in records))
        s.uc.mem_write(0x38860+0xb800,struct.pack('<HHH',count,0,0x6000))
        s.call(0x16f31,[0xb800]);after=bytes(s.uc.mem_read(0x60000,count*17))
        output+=struct.pack('<H',count)+b''.join(struct.pack('<iiiHHB',*record) for record in records)+after
    (ROOT/'tests/reference/airfield.bin').write_bytes(output)
    survey=(ROOT.parent/'corncob-reverse/analysis/native-campaign/airfield-morning.inf').read_bytes()
    (ROOT/'tests/reference/airfield-morning.inf').write_bytes(survey)
    (ROOT/'tests/reference/airfield.json').write_text(json.dumps(dict(reference_sha256=hashlib.sha256(exe).hexdigest(),fixture_sha256=hashlib.sha256(output).hexdigest(),descriptions=len(cases),grouping_cases=96,complete_survey_sha256=hashlib.sha256(survey).hexdigest(),routines=['0x1725f object description/range/bearing','0x16f31 grouping','0x16e32 comparator','0x7ef2 Borland qsort'],stubs=['fprintf output sink','malloc/free backing memory']),indent=2)+'\n')
    print(len(cases),'descriptions and 96 complete grouping/sort cases')

if __name__=='__main__':main()
