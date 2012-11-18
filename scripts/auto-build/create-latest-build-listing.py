#!/usr/bin/python

import string
import os,sys,errno
import re
import StringIO


for root, dirs, files in os.walk('/var/www/auto-build/'):
        dirs.sort()
        if 'latest' in dirs:
                dirs.remove('latest')
        for name in files:
                m = re.search('^Pd-[01]\.[0-9][0-9].*[^5]$', name)
                if m:
                        commonbuildname = re.sub('-20[01][0-9][01][0-9][0-9][0-9]', '', name)
                        commonbuildpath = os.path.join('/var/www/auto-build/latest', commonbuildname)
                        buildtarball = os.path.join(root, m.string)
                        #print "link: ", buildtarball, commonbuildpath
                        try:
                                os.symlink(buildtarball, commonbuildpath)
                        except OSError, e:
                                if e.errno == errno.EEXIST:
                                        #print "removing ", commonbuildpath
                                        os.remove(commonbuildpath)
                                        os.symlink(buildtarball, commonbuildpath)

                                        
