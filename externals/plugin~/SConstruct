import glob
import os
import re
prefix = "/usr/local/lib/pd"

env = Environment(CPPPATH = Split(prefix + '/src /usr/include /usr/local/include . ../../pd/src ../../src src'), CPPDEFINES=['PD','UNIX'], SHLIBPREFIX = '', SHLIBSUFFIX = '.pd_linux', CCFLAGS = '-pipe -O2 -g')

#check for headers
conf = Configure(env)
if conf.CheckCHeader('locale.h'):
  env.Append(CPPDEFINES='HAVE_LOCALE_H')

external = env.SharedLibrary('plugin~',glob.glob('*.c'))
env.Alias('install', env.Install(os.path.join(prefix, 'extra'), external))
env.Alias('install', env.Install(os.path.join(prefix, 'doc/5.reference'),'plugin~-help.pd'))
Default(external)
