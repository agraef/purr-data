#!/usr/bin/python

import string 
import os,sys
import re
import StringIO

library = os.getcwd().split('/')[-1]

print "current library: " + library + "\n"

for root, dirs, files in os.walk('.'):
    try:
        dirs.remove('.svn')
    except:
        pass
#    print "root: " + root
    for name in files:
        m = re.search(".*\.pd$", name)
        if m:
            helppatch = os.path.join(root, m.string)
            fd = open(helppatch, 'r')
            contents = fd.readlines()
            fd.close()
            firstline = contents[0]
            contents.remove(firstline)
#            fd = open(helppatch + ".new", 'w')
            print helppatch
            fd = open(helppatch, 'w')
            fd.write(firstline)
#            fd.write("#X declare -lib " + library.lower() + ";\n")
            fd.write("#X declare -path ..;\n")
            fd.writelines(contents)
            fd.close()

