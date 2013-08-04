#!/usr/bin/python

import re

f = open('../+start-here.pd', 'r')
text = ''
for line in f.readlines():
    text += line

print """# SOME DESCRIPTIVE TITLE.
# This file is put in the public domain.
# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.
#
#, fuzzy
msgid ""
msgstr ""
"Project-Id-Version: Pure Data 0.43\\n"
"Report-Msgid-Bugs-To: http://bugs.puredata.info\\n"
"POT-Creation-Date: 2012-12-31 20:40-0500\\n"
"PO-Revision-Date: 2012-12-31 20:45-0500\\n"
"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n"
"Language-Team: LANGUAGE <LL@li.org>\\n"
"Language: \\n"
"MIME-Version: 1.0\\n"
"Content-Type: text/plain; charset=utf-8\\n"
"Content-Transfer-Encoding: 8bit\\n"
"""

regex = re.compile('#X text [0-9]+ [0-9]+ ([^;]*);', re.MULTILINE | re.DOTALL)
matches = [m.groups() for m in regex.finditer(text)]

nonewlines = re.compile('\n', re.MULTILINE)
nobackslashes = re.compile(' \\\\', re.MULTILINE)
escapedoublequote = re.compile('[^\\\\]"', re.MULTILINE)

ids = []
for m in matches:
    if m:
        chunk = m[0]
        chunk = re.sub(nonewlines, ' ', chunk)
        chunk = re.sub(nobackslashes, '', chunk)
        chunk = re.sub(escapedoublequote, '\\"', chunk)
        if chunk not in ids:
            ids.append(chunk)

for id in ids:
        print '\nmsgid "' + id + '"'
        print 'msgstr ""'
