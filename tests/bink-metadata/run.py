#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Bounded BIKi metadata tests; FFprobe optional and only on original data."""
import argparse,hashlib,json,struct,sys
from pathlib import Path
R=Path(__file__).resolve().parents[2];sys.path.insert(0,str(R/'tests/ppc'));from watchdog import run
p=argparse.ArgumentParser();p.add_argument('--workdir',type=Path,required=True);p.add_argument('--ffprobe',type=Path);a=p.parse_args();w=a.workdir.resolve()
if w==R or R in w.parents:p.error('fresh external output required')
w.mkdir(parents=True,exist_ok=False);records=[];rows=[]
def execute(cmd,label):
 r=run(list(map(str,cmd)),w,label,30);records.append(r)
 if r['timeout'] or r['exit_code']:raise RuntimeError(label+' failed; bounded diagnostics retained')
 return (w/(label+'.stdout')).read_text()
fixture=w/'original.bik';fixture.write_bytes(struct.pack('<21I',0x694b4942,152,3,40,3,640,360,30000,1001,0,2,0x20000,0x10000,0x3000bb80,0x1000ac44,17,23,85,104,143,160)+bytes(76))
# Fixed independent field oracle; Python hashlib verifies the entire canonical
# parsed index and descriptor identities, not a second invocation of C++ code.
canonical=b''.join(struct.pack('<IIB',*f) for f in [(84,20,1),(104,38,0),(142,18,1)])+struct.pack('<8I',0x20000,17,48000,0x3000,0x10000,23,44100,0x1000)
expected={'extent':160,'frames':3,'tracks':2,'width':640,'height':360,'rate_num':30000,'rate_den':1001,'metadata_bytes':84,'payload_bytes_read':0,'index_sha256':hashlib.sha256(canonical).hexdigest()}
for profile,flags in [('Debug',['-O0']),('Release',['-O2']),('Sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])]:
 for src in ['contract','inspect']:
  binary=w/(src+'-'+profile);execute(['clang++','-std=c++23','-Wall','-Wextra','-Werror','-pthread',*flags,'-iquote',R,R/'tests/bink-metadata'/(src+'.cpp'),'-o',binary],'compile-'+src+'-'+profile)
  row=json.loads(execute([binary,*([fixture] if src=='inspect' else [])],'run-'+src+'-'+profile))
  if src=='inspect':assert row==expected
  rows.append({'profile':profile,'suite':src,**row});print(json.dumps(rows[-1]),flush=True)
independent=None
if a.ffprobe:
 tool=a.ffprobe.resolve();version=execute([tool,'-version'],'ffprobe-version').splitlines()[0]
 independent=json.loads(execute([tool,'-v','error','-nofind_stream_info','-f','bink','-show_entries','stream=codec_type,width,height,avg_frame_rate,sample_rate,channels','-of','json',fixture],'ffprobe-metadata'))
 v,*tracks=independent['streams'];assert (v['width'],v['height'],v['avg_frame_rate'])==(640,360,'30000/1001');assert [(int(t['sample_rate']),t['channels']) for t in tracks]==[(48000,2),(44100,1)];independent['version']=version
(w/'results.json').write_text(json.dumps({'tests':rows,'records':records,'original_fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),'independent_ffprobe':independent,'payload_decoded':False,'title_input':False},indent=2)+'\n')
