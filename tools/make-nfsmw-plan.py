"""Write assets/plans/NFSMW.json (Prospero BlackBox) from the private repo's records.

    make_plan.py OUT.json PRIVATE_REPO [--upgrade | --rtpatch SHA256 SIZE]

The plan is vanilla 1.3 with the quality-of-life mods: the DRM-free exe, ASI loader,
XtendedInput, Widescreen Fix, HUD Adapter, DSOAL and our console keyboard. Its one option,
"x360" (chosen at Install, off by default), adds the Xbox 360 setup the console ran on
2026-10-11: the Stuff Pack and the converted movies (repacked by tools/make-x360-assets.py,
profiles/nfsmw/x360-assets.json), HD Content Support, Front-End Shadows, HD Reflections,
native D3DX and our async-luminance ASI (tools/build-x360-bits.sh). It draws with the
prefix's own DXVK.
Files this repo provides are read from it (profiles/nfsmw/built/, mods/, release/x360/,
release/settings-mod/), so
their hashes follow what is committed and built.

PRIVATE_REPO is a checkout of lavavex/nfs-blackbox-ps5 (profiles/nfsmw/, tools/nfsmw-x360.json,
mods/). --upgrade adds the 1.2 -> 1.3 diff in this repo (profiles/nfsmw/nfsmw-1.2-to-1.3.zip, made
by prospero-blackbox's tools/make-upgrade.py); --rtpatch adds EA's own Patch.v1.3.exe
instead, which Prospero BlackBox reads without running it.
"""
import argparse,json,re,zipfile,io,hashlib,sys,os
args=argparse.ArgumentParser(); args.add_argument('out'); args.add_argument('repo')
how=args.add_mutually_exclusive_group()
how.add_argument('--upgrade',action='store_true')
how.add_argument('--rtpatch',nargs=2,metavar=('SHA256','SIZE'))
args=args.parse_args()
R=os.path.join(os.path.abspath(args.repo),'')
man={r['path']:r for r in json.load(open(R+'profiles/nfsmw/game-manifest.json'))['files']}
assets=json.load(open(R+'profiles/nfsmw/x360-assets.json'))
def sha(b): return hashlib.sha256(b).hexdigest()
def zipsrc(path): b=open(R+path,'rb').read(); return sha(b),len(b)
S={}
def source(id,name,sha256,size,url=None,drive=None):
    S[id]={"name":name,"sha256":sha256,"size":size}
    if url: S[id]["url"]=url
    if drive: S[id]["drive"]=drive
G=[]; XG=[]
def place(src,to,member=None,into=None):
    p={"source":src,"to":to}
    if member: p["member"]=member
    (G if into is None else into).append(p)
exe=open(R+'profiles/nfsmw/built/speed.exe','rb').read()
assert sha(exe)=="80774c2e5d619b4f120b48d4462896fd504c263399d203a238769cffde1d253c"
source("speed-exe","speed.exe 1.3, DRM-free (the *!ReLOADeD!* build; nfs-blackbox-ps5: profiles/nfsmw/built/speed.exe)",sha(exe),len(exe))
place("speed-exe","speed.exe")
source("asi-loader","Ultimate ASI Loader 9.7.4 (ThirteenAG)","952cebfc30d525afc2bdbaca954329d405ded3aa688a83027354dae14dfd5c5f",3910913,
       "https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/download/v9.7.4/Ultimate-ASI-Loader.zip")
place("asi-loader","dinput8.dll","dinput8.dll")
source("xtendedinput","NFS-XtendedInput 1.22 for Most Wanted (xan1242)",*zipsrc('mods/Release-MW-Pack.zip'),
       "https://github.com/xan1242/NFS-XtendedInput/releases/download/1.22/Release-MW-Pack.zip")
place("xtendedinput","scripts/","scripts/"); place("xtendedinput","GLOBAL/","GLOBAL/")
source("widescreenfix","Most Wanted Widescreen Fix, build of 2026-09-12 (ThirteenAG)",*zipsrc('mods/NFSMostWanted.WidescreenFix.zip'),
       "https://github.com/ThirteenAG/WidescreenFixesPack/releases/download/nfsmw/NFSMostWanted.WidescreenFix.zip")
place("widescreenfix","scripts/","scripts/")
source("hud-adapter","NFSMW HUD Adapter 1.0.0.800 (nlgzrgn)",*zipsrc('mods/companions/658865-NFSMW HUD Adapter v1.0.0.800.zip'),
       "https://nfsmods.xyz/uploads/mods/638/658865-NFSMW%20HUD%20Adapter%20v1.0.0.800.zip")
place("hud-adapter","scripts/NFSMWHUDAdapter.asi","scripts/NFSMWHUDAdapter.asi")
outer=zipfile.ZipFile(R+'mods/dsoal/DSOAL.zip'); inner=outer.read('DSOAL_r648.zip')
source("dsoal","DSOAL r648 (kcat): DSOAL_r648.zip, from inside DSOAL.zip on github.com/kcat/dsoal/releases/tag/r648 (nfs-blackbox-ps5: mods/dsoal/DSOAL.zip)",sha(inner),len(inner))
for n in ("dsound.dll","dsoal-aldrv.dll","alsoft.ini"): place("dsoal",n,"DSOAL/Win32/"+n)
# Our console keyboard, as GitHub Actions builds and releases it (repeatable: make in the
# repo with the release notes' compiler gives these bytes).
source("console-keyboard","NFSMWConsoleKeyboard.asi v1.1 (lavavex/NFSMW-ConsoleKeyboard, built by GitHub Actions)",
       "b65b1c745c32cfc7fb9b6ad41eeb3487e0649068731a29273d8e6006b5b9d1ec",45568,
       "https://github.com/lavavex/NFSMW-ConsoleKeyboard/releases/download/v1.1/NFSMWConsoleKeyboard.asi")
place("console-keyboard","scripts/NFSMWConsoleKeyboard.asi")
# The PS5 app's settings in the game's own Options menu (PS5 Options), built by this repo.
# (It replaced the ImGui menu, mods/NFSBlackBoxMenu, on 2026-10-17.)
settings_mod=open(R+'release/settings-mod/PS5SettingsMod.asi','rb').read()
source("ps5-settings-mod","PS5SettingsMod.asi (nfs-blackbox-ps5 mods/ps5-settings-mod, built by tools/build-settings-mod.sh)",sha(settings_mod),len(settings_mod))
place("ps5-settings-mod","scripts/PS5SettingsMod.asi")
system=[]
inis=R+'profiles/nfsmw/inis/'
stock_alsoft=zipfile.ZipFile(io.BytesIO(inner)).read('DSOAL/Win32/alsoft.ini')
console_alsoft=open(inis+'alsoft.ini','rb').read()
assert console_alsoft.startswith(stock_alsoft)
appended=console_alsoft[len(stock_alsoft):].decode('ascii')
texts=[{"to":"dxvk.conf","body":open(inis+'dxvk.conf','rb').read().decode('utf-8')},
       {"to":"alsoft.ini","body":appended,"append":True}]
if args.upgrade:
    diff=open(R+'profiles/nfsmw/nfsmw-1.2-to-1.3.zip','rb').read()
    source("upgrade-1.2","Most Wanted 1.2 to 1.3 diff (nfs-blackbox-ps5: profiles/nfsmw/nfsmw-1.2-to-1.3.zip): put it in mods-cache by hand",
           hashlib.sha256(diff).hexdigest(),len(diff))
if args.rtpatch:
    source("patch-1.3","EA's Most Wanted Patch.v1.3.exe (the same for every region): put it in mods-cache by hand",
           args.rtpatch[0],int(args.rtpatch[1]))
for t in texts: assert all(ord(c)<0x80 or t['to']=='dxvk.conf' for c in t['body'])
# UseWin32Cursor=1: the game doesn't draw its cursor, and Wine on the PS5 draws none (owner,
# 2026-10-17: the cursor sat in the menus' top left corner); the touchpad block below, which
# the launcher now always applies, sets it back to 0. EnableMouseHiding=1: with UseWin32Cursor the Win32 cursor still sat in
# the middle of the screen in a race (owner, 2026-10-18); XtendedInput hides it after 5 s
# without a move, and with Touchpad it comes back at a touch.
ini=[{"file":"scripts/NFS_XtendedInput.ini","values":{"UseWin32Cursor":"1","UseCustomCursor":"0","EnableMouseHiding":"1","ControllerIconMode":"1","FirstControlDevice":"1"}},
     {"file":"scripts/NFSMostWanted.WidescreenFix.ini","lf":True,"values":{"FixHUD":"0","Scaling":"0","FMVWidescreenMode":"0","SkipIntro":"0","ImproveGamepadSupport":"3",
        "ImproveShadowLOD":"0","AutoScaleShadowsRes":"0","DisableMotionBlur":"1","ConsoleGamma":"2","SMAA":"1"}},
     ]
# XtendedInput's ini is not expected to equal the console's any more: its cursor line differs.
expect={p:man[p]['sha256'] for p in ["speed.exe","dxvk.conf","alsoft.ini","scripts/NFSMostWanted.WidescreenFix.ini"]}
def regblock(path):
    out=[]
    for line in open(path).read().splitlines():
        m=re.match(r'^"([^"]+)"=(dword:([0-9a-f]{8})|"(.*)")$',line)
        if not m: continue
        v={"key":"Software\\EA Games\\Need for Speed Most Wanted" if os.path.basename(path).startswith('user') else "Software\\Wow6432Node\\EA Games\\Need for Speed Most Wanted","name":m.group(1)}
        if m.group(3): v["dword"]=int(m.group(3),16)
        else: v["string"]=m.group(4).replace('\\\\','\\')
        out.append(v)
    return out
user=regblock(R+'profiles/nfsmw/user-ea.reg'); machine=regblock(R+'profiles/nfsmw/system-ea.reg')
assert len(user)==28 and len(machine)==4, (len(user),len(machine))
plan={"schema":1,"game":"NFSMW","title":"Need for Speed: Most Wanted (2005)","release":"1.3","prefix":"nfs-blackbox",
 "_about":(["Most Wanted 1.3 with the quality-of-life mods: the DRM-free exe with the frame-limiter and",
   "large-address-aware patches, the ASI loader, XtendedInput (DualSense, PlayStation prompts), the",
   "Widescreen Fix (motion blur off), HUD Adapter, DSOAL (EAX audio) and the console keyboard.",
   "prospero-win's own DXVK draws it."])+
  ["Every file is pinned by SHA-256 and taken from mods-cache/<sha256>[.ext]; the edited files are",
   "checked against the console's own (expect)."],
 "sources":S,"game_files":G,"system_files":system,
 "patches":[{"file":"speed.exe","offset":0x242ED2,"from":"e829a00000","to":"9090909090","_why":"the frame limiter's call"},
            {"file":"speed.exe","offset":0x11E,"from":"0f","to":"2f","_why":"large address aware"}],
 "ini":ini,"texts":texts,"expect":expect,
 "registry":{"machine":machine,"user":user},
 "profile":{"id":"pb-nfsmw","name":"NFS Most Wanted","executable":"C:\\Games\\NFSMW\\speed.exe","working_directory":"C:\\Games\\NFSMW",
   "dll_overrides":"d3d8,d3d9,d3d10core,d3d11,dxgi=n;dinput8,dsound=n,b","width":3840,"height":2160,
   "runtime":{"fast_clock":"true","shared_input":"true","thread_scheduling":"true"},"after":"nfs-most-wanted-2005.profile"},
 "pending":[],
 # The touchpad cursor, always applied now (Off is no longer offered: Windows' cursor still
 # showed in cutscenes): XtendedInput draws the game's cursor again and prospero-win moves it
 # with the touchpad (the profile's [input] mouse = touchpad).
 "touchpad":{"ini":[{"file":"scripts/NFS_XtendedInput.ini","values":{"UseWin32Cursor":"0"}}]}}
# The Xbox 360 option. Its sources join the plan's list after the base ones; the manager
# fetches a source only when a chosen placement uses it.
for z in assets["stuff_pack"]+assets["movies"]:
    sid=z["name"][:-len(".zip")]
    source(sid,"Xbox 360 "+("Stuff Pack 4.2" if z in assets["stuff_pack"] else "Converted Movies")+" ("+z["name"]+", repacked from Google Drive by nfs-blackbox-ps5 tools/make-x360-assets.py)",z["sha256"],z["size"],z["url"])
    for m in z["members"]: place(sid,m["path"],m["path"],XG)
source("hd-content-support","NFS HD Content Support (Aero_)",*zipsrc('mods/companions/NFS HD Content Support.zip'),
       "https://nfsmods.xyz/uploads/mods/2151/4c35cb-NFS%20HD%20Content%20Support.zip")
place("hd-content-support","scripts/NFSMWHDContentSupport.asi","NFS HD Content Support/NFSMWHDContentSupport/scripts/NFSMWHDContentSupport.asi",XG)
source("fe-shadows","NFSMW Front-End Shadows (Aero_)",*zipsrc('mods/companions/9cbcd3-NFSMWFEShadows.zip'),
       "https://nfsmods.xyz/uploads/mods/3005/9cbcd3-NFSMWFEShadows.zip")
place("fe-shadows","scripts/NFSMWFEShadows.asi","NFSMWFEShadows/scripts/NFSMWFEShadows.asi",XG)
place("fe-shadows","scripts/NFSMWFEShadows.ini","NFSMWFEShadows/scripts/NFSMWFEShadows.ini",XG)
source("hd-reflections","NFS HD Reflections (Aero_)",*zipsrc('mods/companions/a486f3-NFS HD Reflections.zip'),
       "https://nfsmods.xyz/uploads/mods/3363/a486f3-NFS%20HD%20Reflections.zip")
place("hd-reflections","scripts/","NFS HD Reflections/NFSMWHDReflections/scripts/",XG)
built=R+'release/x360/'  # tools/build-x360-bits.sh
for sid,f,h in [("async-luminance","X360StuffAsyncLuminance.asi","98f7b9338ecf038ec623df128e00ad8e94dbea3d9908b394fc36cd12352b46fa"),
                ("d3dx9_26","d3dx9_26.dll","8ea96fe01c3c86a36fcb3795ae03eb12034003e335ef475571efaeda17c5bc78"),
                ("d3dx9_43","d3dx9_43.dll","0b28546be22c71834501f7d7185ede5d79742457331c7ee09efc14490dd64f5f")]:
    b=open(built+f,'rb').read(); assert sha(b)==h,(f,sha(b))
    source(sid,{"async-luminance":"X360StuffAsyncLuminance.asi (nfs-blackbox-ps5, built by tools/build-x360-bits.sh)"}.get(sid,
           f+" (x86, Microsoft DirectX June 2010 redistributable; nfs-blackbox-ps5 tools/build-x360-bits.sh)"),h,len(b))
place("async-luminance","scripts/X360StuffAsyncLuminance.asi",into=XG)
x360={"id":"x360","title":"Xbox 360 graphics",
 "description":"The Xbox 360 look: the X360 Stuff Pack 4.2 (no glass shader, which smears under DXVK), the converted Xbox 360 movies, HD Content Support, Front-End Shadows, HD Reflections, native D3DX and an async-luminance fix. About 3.7 GB more to download.",
 "default":False,"game_files":XG,
 "system_files":[{"source":s,"to":"syswow64/"+s+".dll","keep_builtin":True} for s in ("d3dx9_26","d3dx9_43")],
 "texts":[{"to":"scripts/X360Stuff.ini","body":open(inis+'X360Stuff.ini','rb').read().decode('latin-1')}],
 "ini":[{"file":"scripts/NFSMWFEShadows.ini","values":{"RestoreShadows":"1"}},
        {"file":"scripts/NFSMWHDReflections.ini","values":{"CubemapBrightnessFix":"0","ForceEnableMirror":"0","RestoreWaterReflections":"1"}}],
 "expect":{p:man[p]['sha256'] for p in ["scripts/X360Stuff.ini","scripts/NFSMWFEShadows.ini","scripts/NFSMWHDReflections.ini"]},
 "dll_overrides":";d3dx9_26,d3dx9_43=n"}
movies={r['path'] for r in man.values() if r['mod']=='360 Converted Movies'}
assert movies<= {p['to'] for p in XG} and len(movies)==33
plan["options"]=[x360]
if args.upgrade: plan["upgrades"]=[{"from":"1.2","source":"upgrade-1.2"}]
if args.rtpatch: plan["upgrades"]=[{"from":"1.2","source":"patch-1.3","kind":"rtpatch"}]
open(args.out,'w').write(json.dumps(plan,indent=1,ensure_ascii=False)+"\n")
print(len(S),'sources',len(G),'game files',sum(v['size'] for v in S.values())/1e9,'GB')
