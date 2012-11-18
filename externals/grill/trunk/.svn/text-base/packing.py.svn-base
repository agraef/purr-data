import string
import sys
import os
import shutil
import fnmatch
import glob
from types import TupleType, ListType

# sys.path += ['/Volumes/Daten/Prog/packs/Python-2.2.2/Mac/Lib']
try:
	import macostools
except:
	pass

folders = {
    "flext": "flext",
    "flext-tut": "flext/tutorial",
    "vasp": "vasp", "vasp-bin": "vasp",
    "pool": "pool", "pool-bin": "pool",
    "py": "py", "py-bin": "py",
    "xsample": "xsample", "xsample-bin": "xsample",
    "dynext": "dynext", "dynext-bin": "dynext",
    "wmangle-bin": "wmangle",
}

stdfiles = ["gpl.txt","license.txt","readme.txt","changes.txt","notes.txt","build.txt"]

autoconffiles = [] # ["bootstrap.sh","configure.ac","Makefile.am"]
    
buildfiles = ["build.sh","build.bat","build","package.txt"]

flexttut_s = [
    "simple1","simple2","simple3",
    "adv1","adv2","adv3",
    "attr1","attr2","attr3",
    "signal1","signal2",
    "sndobj1",
    "stk1","stk2",
    "lib1",
    "bind1",
    "buffer1",
    "timer1"
]
flexttut_t = [
    "thread1","thread2",
]
flexttut = flexttut_s+flexttut_t

flextbld = ["buildsys/*"]

modules = {
    "flext": [stdfiles], 
    "flext bld pd win": [buildfiles,autoconffiles,flextbld],
    "flext bld pd lnx": [buildfiles,autoconffiles,flextbld],
    "flext bld pd osx": [buildfiles,autoconffiles,flextbld],
    "flext bld max win": [buildfiles,autoconffiles,flextbld],
    "flext bld max os9": [],
    "flext bld max osx": [buildfiles,autoconffiles,flextbld],
    "flext doc": ["Doxyfile"],
    "flext src": [autoconffiles,"source/*.h","source/*.cpp"],

    "flext-tut": [stdfiles],
    "flext-tut bld pd win": [buildfiles,autoconffiles],
    "flext-tut bld pd lnx": [buildfiles,autoconffiles],
    "flext-tut bld pd osx": [buildfiles,autoconffiles],
    "flext-tut bld max os9": [x+"/"+x+".mcp" for x in flexttut_s],
    "flext-tut bld max osx": [buildfiles,autoconffiles],
    "flext-tut bin pd win": ["pd-msvc/*.dll"],
    "flext-tut bin pd lnx": ["pd-linux/*.pd_linux"],
    "flext-tut bin pd osx": ["pd-darwin/*.pd_darwin"],
    "flext-tut bin max os9": ["max-os9/"+x for x in flexttut_s,"max-os9/"+x+"~" for x in flexttut_s], 
    "flext-tut bin max osx": ["max-osx/"+x for x in flexttut,"max-osx/"+x+"~" for x in flexttut], 
    "flext-tut doc pd": ["pd/*.pd"],
    "flext-tut doc max": ["maxmsp/*"],
    "flext-tut src": [(x+"/main.cpp",x+"/package.txt") for x in flexttut],

    "vasp": [stdfiles,"mixfft.txt"],
    "vasp bld pd win": [buildfiles,autoconffiles],
    "vasp bld pd lnx": [buildfiles,autoconffiles],
    "vasp bld pd osx": [buildfiles,autoconffiles],
    "vasp bld max win": [buildfiles,autoconffiles],
    "vasp bld max osx": [buildfiles,autoconffiles],
    "vasp bin pd win": ["pd-msvc/release-multi/vasp.dll"],
    "vasp bin pd osx": ["pd-darwin/release-multi/vasp.pd_darwin"],
    "vasp bin max osx": ["max-darwin/release-multi/vasp.mxo","max-darwin/vasp.mxd"],
    "vasp bin max win": ["max-msvc/release-multi/vasp.mxe"],
    "vasp src": ["source/*.h","source/*.cpp"],
    "vasp doc pd": ["pd-help/*"],
#    "vasp doc max": ["max-help/*"],
    "vasp extra pd": ["pd/*"],
    "vasp extra max": ["maxmsp/*"],

    "pool": stdfiles,
    "pool bld pd win": [buildfiles,autoconffiles],
    "pool bld pd lnx": [buildfiles,autoconffiles],
    "pool bld pd osx": [buildfiles,autoconffiles],
    "pool bld max win": [buildfiles,autoconffiles],
    "pool bld max osx": [buildfiles,autoconffiles],
    "pool bin pd win": ["pd-msvc/release-single/pool.dll"],
    "pool bin pd osx": ["pd-darwin/release-single/pool.pd_darwin"],
    "pool bin max win": ["max-msvc/release-single/pool.mxe"],
    "pool bin max osx": ["max-darwin/release-single/pool.mxo","max-darwin/pool.mxd"],
    "pool doc pd": ["pool-help.pd"],
    "pool doc max": ["pool.help"],
    "pool extra": ["pool-*.dtd"],
    "pool src": ["source/*.h","source/*.cpp"],

    "py": stdfiles,
    "py bld pd win": [buildfiles,autoconffiles],
    "py bld pd lnx": [buildfiles,autoconffiles],
    "py bld pd osx": [buildfiles,autoconffiles],
    "py bld max win": [buildfiles,autoconffiles],
    "py bld max osx": [buildfiles,autoconffiles],
    "py bin pd win": ["pd-msvc/release-multi/py.dll"],
    "py bin max win": ["maxmsp/py-objectmappings.txt","max-msvc/release-multi/py.mxe"],
    "py bin pd osx": ["pd-darwin/release-multi/py.pd_darwin"],
    "py bin max osx": ["maxmsp/py-objectmappings.txt","max-darwin/release-multi/py.mxo"],
    "py doc pd": ["pd/*.pd","scripts/*.py"],
    "py doc max": ["maxmsp/*.pat","maxmsp/*.mxb","maxmsp/*.mxt","scripts/*.py"],
    "py src": ["source/*.h","source/*.cpp"],

    "xsample": stdfiles,
    "xsample bld pd win": [buildfiles,autoconffiles],
    "xsample bld pd lnx": [buildfiles,autoconffiles],
    "xsample bld pd osx": [buildfiles,autoconffiles],
    "xsample bld max win": [buildfiles,autoconffiles],
    "xsample bld max osx": [buildfiles,autoconffiles],
    "xsample bin pd win": ["pd-msvc/release-single/xsample.dll"],
    "xsample bin pd osx": ["pd-darwin/release-single/xsample.pd_darwin"],
    "xsample bin max osx": ["max-darwin/xsample.mxd"],  # "max-darwin/release-single/xsample.mxo",
    "xsample bin max win": ["max-msvc/release-single/xsample.mxe"],
    "xsample doc pd": ["pd/*.pd","pd-ex/*.pd"],
    "xsample doc max": ["maxmsp/*.help","maxmsp/*.txt"],
    "xsample src": ["source/*.h","source/*.cpp"],
    
    "dynext": stdfiles,
    "dynext bld pd win": [buildfiles,autoconffiles],
    "dynext bld pd lnx": [buildfiles,autoconffiles],
    "dynext bld pd osx": [buildfiles,autoconffiles],
    "dynext bin pd win": ["pd-msvc/release-single/dyn~.dll"],
    "dynext bin pd osx": ["pd-darwin/release-single/dyn~.pd_darwin"],
    "dynext doc pd": ["pd/dyn~-help.pd"],
    "dynext src": ["src/*.h","src/*.cpp"],

    "wmangle": stdfiles,
    "wmangle bld pd win": [buildfiles,autoconffiles],
    "wmangle bld pd osx": [buildfiles,autoconffiles],
    "wmangle bld max win": [buildfiles,autoconffiles],
    "wmangle bld max os9": ["wmangle.mcp"],
    "wmangle bld max osx": [buildfiles,autoconffiles],
    "wmangle bin pd win": ["pd-msvc/wmangle.dll"],
    "wmangle bin pd osx": ["pd-darwin/wmangle.pd_darwin"],
    "wmangle bin max win": ["max-msvc/wmangle.mxe"],
    "wmangle bin max os9": ["max-os9/wmangle"],
    "wmangle bin max osx": ["max-osx/wmangle.mxd"],
    "wmangle doc pd": ["wmangle-help.pd"],
    "wmangle doc max": ["wmangle.help"],
    "wmangle src": ["*.h","*.cpp"],

    "vst": stdfiles,
    "vst bld pd win": [buildfiles,autoconffiles],
    "vst bld pd osx": [buildfiles,autoconffiles],
    "vst bin pd win": ["pd-msvc/vst~.dll"],
    "vst bin pd osx": ["pd-darwin/vst~.pd_darwin"],
    "vst doc pd": ["vst~-help.pd"],
    "vst src": ["*.h","*.cpp"],
}

stddist = { "src": ["bld","src","doc","extra"], "bin": ["bin","doc","extra"] }

dists = {
    "flext": ("flext",stddist["src"]),
    
    "flext-tut": ("flext-tut",stddist["src"]),

    "vasp": ("vasp",stddist["src"]),
    "vasp-bin": ("vasp",stddist["bin"]),

    "pool": ("pool",stddist["src"]),
    "pool-bin": ("pool",stddist["bin"]),

    "py": ("py",stddist["src"]),
    "py-bin": ("py",stddist["bin"]),

    "xsample": ("xsample",stddist["src"]),
    "xsample-bin": ("xsample",stddist["bin"]),

    "dynext": ("dynext",stddist["src"]),
    "dynext-bin": ("dynext",stddist["bin"]),

    "vst": ("vst",stddist["src"]),
    "vst-bin": ("vst",stddist["bin"]),

    "wmangle-bin": ("wmangle",stddist["bin"]),
}

###################################################################################


def make_win(dstfile,srclst):
    f = dstfile+".zip"
    try:
        os.remove(f)
    except:
        pass

    r = 0
    l = len(srclst)
    sl = 20 # chunk size

    for i in range(0,l-1,sl):
        args = ["wzzip","-r -P "+f]+srclst[i:i+sl]
        r = os.spawnv(os.P_WAIT,"c:\programme\util\winzip\wzzip",args)
        if r != 0:
        	break
    if r == 0:
        return f
    else:
        print "wzzip could not be executed"
        return ""
    
def make_lnx(dstfile,srclst):
    f = dstfile+".tgz"
	
    args = ["tar","-c","-z","--exclude",".*","-f"+f]+srclst
    try:
        os.remove(f)
    except:
        pass
        
    r = os.spawnv(os.P_WAIT,"/bin/tar",args)
    if r == 0:
        return f
    else:
        print "tar could not be executed"
        return ""

def make_osx(dstfile,srclst):
    f = dstfile+".tgz"
	
    args = ["tar","-c","-z","--exclude",".*","-f"+f]+srclst
    try:
        os.remove(f)
    except:
        pass
        
    r = os.spawnv(os.P_WAIT,"/usr/bin/"+args[0],args)
    if r == 0:
        print "ATTENTION: Resource forks are not preserved!"
        return f
    else:
        print args[0]+" could not be executed"
        return ""

def make_os9(dstfile,srclst):
    try:
        shutil.rmtree(dstfile)
    except:
        pass
    
    os.mkdir(dstfile)
    for s in srclst:
        dx = dstfile+"/"+os.path.dirname(s)
        try:
            os.makedirs(dx)
        except:
            pass
        shutil.copy(s,dx)
        
    return dstfile

makes = {
    "win": make_win,
    "lnx": make_lnx,
    "osx": make_osx,
    "os9": make_os9,
}

def flatten(seq):
    res = []
    for item in seq:
        if type(item) in (TupleType, ListType):
            res.extend(flatten(item))
        else:
            res.append(item)
    return res

def joinpath(lst):
    return reduce(os.path.join,lst)

#def recpath(plst,pre = []):
#    print len(pre)
#    
#    ret = []
#    if len(plst) > 0:
#        abspath = os.getcwd()
#        dir = os.listdir(abspath)
#        fdir = fnmatch.filter(dir,plst[0])
##        print "DIR",plst[0],fdir
#        for l in fdir:
#            if os.path.isfile(l):
#                if len(pre) > 0:
#                    p = pre
#                    pre.append(l)
#                    ret.append(joinpath(p))
#                else:
#                    ret.append(p)
#            elif os.path.isdir(l):
#                os.chdir(l)
#                ret.extend(recpath(plst[1:],pre.append(l)))
#                os.chdir(abspath)
#    return ret            

if __name__ == "__main__":
    if len(sys.argv) != 5:
        print "Syntax: packing.py target(flext,vasp,...) system(pd/max) platform(win,lnx,osx,os9) version"
        sys.exit()

    target = sys.argv[1]
    system = sys.argv[2]
    platform = sys.argv[3]
    version = sys.argv[4]

#    print "Packing for ",target,system,platform,version
        
    try:
        dist = dists[target]
    except:
        print "Target package unknown"
        sys.exit()

    try:
        folder = folders[target]
    except:
        print "Target folder unknown"
        sys.exit()

    try:
        make = makes[platform]
    except:
        print "Packing procedure unknown"
        sys.exit()

### generate file list #############

#   start with common files

    files = []
    try:
        files.extend(modules[dist[0]])
    except:
        pass
    
#    print "Files1:", files

#   add system/platform specific files

    for d in dist[1]:
        m = dist[0]+" "+d
        try:
            files.extend(modules[m])
        except:
            pass
        try:
            files.extend(modules[m+" "+system])
        except:
            pass
        try:
            files.extend(modules[m+" "+system+" "+platform])
        except:
            pass

#   replace folder delimiters, expand wildcards in file names

    files = flatten(files) # make a shallow copy of file list

#    print "Files:", files

    xfiles = []
    for f in files:
        spl = string.split(folder+"/"+f,"/")
        x = glob.glob(joinpath(spl))
        xfiles.extend(x)

### call the packer procedure ###############
        
    print "Files:", xfiles

    path = target+"-"+version+"-"+system+"-"+platform
    pack = make(path,xfiles)

#   done!

    if len(pack) != 0:
        print "The package has been successfully packed to",pack
