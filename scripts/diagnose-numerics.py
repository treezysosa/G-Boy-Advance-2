from pathlib import Path
import shutil,subprocess,json,array,math,hashlib
r=Path.cwd();w=r/'build/numeric-diagnostics';w.mkdir(parents=True,exist_ok=True)
base=r/'build/cpu-baseline-source/Source';current=r/'Source';variants={}
for name in ['baseline','current','legacy-wave','legacy-additive','legacy-stable','legacy-math']:
 d=w/name/'Source';shutil.copytree(base if name=='baseline' else current,d,dirs_exist_ok=True);shutil.copy2(current/'FactoryData.h',d/'FactoryData.h');variants[name]=d
baseline=(base/'dsp/SynthEngine.cpp').read_text();oldcase=baseline[baseline.index('        case 2:{'):baseline.index('        case 3:',baseline.index('        case 2:{'))]
for name,d in variants.items():
 p=d/'dsp/SynthEngine.cpp';s=p.read_text()
 if name in ['legacy-wave','legacy-math']:
  a=s.index('    // Both blends are bounded');b=s.index('\n}',a)
  s=s[:a]+'''    float x=std::lerp(read(a,m),read(a,m2),tuning.blend),y=std::lerp(read(b,m),read(b,m2),tuning.blend);
    return std::lerp(x,y,position-float(a));'''+s[b:]
 if name in ['legacy-additive','legacy-math']:
  a=s.index('        case 2:{');b=s.index('        case 3:',a);s=s[:a]+oldcase+s[b:]
 if name=='legacy-stable':s=s.replace('return movingCount==0&&previewSamples==0\n','return movingCount==0&&previewSamples==0&&(engine==0||engine==2||engine==3)\n')
 p.write_text(s)
lines=['cmake_minimum_required(VERSION 3.24)','project(NumericDiagnostics LANGUAGES CXX)','set(CMAKE_CXX_STANDARD 20)','set(CMAKE_CXX_STANDARD_REQUIRED ON)','set(CMAKE_CXX_EXTENSIONS OFF)','set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")']
for n,d in variants.items():
 safe=n.replace('-','_');rel=d.relative_to(w).as_posix();driver=(r/'Tests/PerformanceProbe.cpp').as_posix()
 lines += [f'add_library(dsp_{safe} STATIC "{rel}/dsp/SynthEngine.cpp" "{rel}/dsp/Effects.cpp")',f'target_include_directories(dsp_{safe} PUBLIC "{rel}")',f'target_compile_options(dsp_{safe} PRIVATE /W4 /permissive-)',f'add_executable(probe_{safe} "{driver}")',f'target_link_libraries(probe_{safe} PRIVATE dsp_{safe})']
(w/'CMakeLists.txt').write_text('\n'.join(lines)+'\n')
subprocess.run(['cmake','-S',str(w),'-B',str(w/'build'),'-G','Visual Studio 17 2022','-A','x64'],check=True)
subprocess.run(['cmake','--build',str(w/'build'),'--config','Release','--parallel','2'],check=True)
for n in variants:subprocess.run([str(w/'build/Release'/('probe_'+n.replace('-','_')+'.exe')),str(w/(n+'.f32'))],check=True)
report={}
for name in list(variants)[1:]:
 hashes=[hashlib.sha256(),hashlib.sha256()];total=changed=worst=0;peak=energy=noise=0
 with (w/'baseline.f32').open('rb') as a,(w/(name+'.f32')).open('rb') as b:
  while True:
   raw,other=a.read(1024*1024),b.read(1024*1024);assert len(raw)==len(other)
   if not raw:break
   hashes[0].update(raw);hashes[1].update(other);x=array.array('f');x.frombytes(raw);y=array.array('f');y.frombytes(other)
   energy+=sum(float(v)*v for v in x)
   if raw!=other:
    for i,(v,z) in enumerate(zip(x,y)):
     assert math.isfinite(v) and math.isfinite(z);d=float(v)-z;noise+=d*d;changed+=v!=z
     if abs(d)>peak:peak=abs(d);worst=total+i
   total+=len(x)
 assert total==12263424
 scene=worst//32768 if worst<3932160 else 120+(worst-3932160)//12288
 bank=json.loads((r/'Presets/factory.json').read_text())['presets']
 label={'sampleRate':[44100,48000,96000][scene//40],'engine':(scene%40)//5,'filter':scene%5} if scene<120 else bank[scene-120]['name']
 snr=None if noise==0 else 10*math.log10(energy/noise)
 report[name]={'maxAbsoluteDifference':peak,'signalToErrorDb':snr,'differentSamples':changed,'worstFloatIndex':worst,'worstScene':scene,'worstSceneDescription':label,'beforeSha256':hashes[0].hexdigest(),'afterSha256':hashes[1].hexdigest(),'passExistingBounds':peak<=1e-6 and (snr is None or snr>=110)}
 print(name,json.dumps(report[name]),flush=True)
(w/'report.json').write_text(json.dumps(report,indent=2)+'\n')
for p in w.glob('*.f32'):p.unlink()
