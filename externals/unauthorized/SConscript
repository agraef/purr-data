import glob
import os
import re
Import('env prefix')

for tk in glob.glob('*/*.tk'):
    (dir, file) = os.path.split(tk)
    filename = re.sub("\.tk$","",file)
    cmd = "./tk2c.bash < " + dir + "/" + file + " > " + dir + "/" + filename + ".tk2c"
    os.popen(cmd)

for extra in glob.glob('*/*.c'):
    unauthorized = env.SharedLibrary(target = re.sub("\.c$","",os.path.basename(extra)), source = extra)
    env.Alias('install', env.Install(os.path.join(prefix, 'extra'), unauthorized))
    Default(unauthorized)

env.Alias('install', env.Install(os.path.join(prefix, 'doc/unauthorized'), glob.glob('*/*.pd')))
