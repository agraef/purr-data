#!/usr/bin/env python

import sys,string

file=open(sys.argv[1],"r")

while 1:
    line=""
    while line=="" or line=="\n" or line[0:1]==";":
        line=file.readline()
        if line=="":
            file.close()
            sys.exit(0)
    line=string.replace(line[:-1],'\\','\\\\')
    sys.stdout.write('"'+string.replace(line,'"','\\"')+'\\n"\n')




