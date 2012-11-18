# PixelTANGO Python code for dirlist and stripExtension
# Copyright Ben Bogart, Franz Hildgen, 
# The Societe des arts technologiques and
# The Interaccess Electronic Media Arts Centre

import os 
import os.path
import glob

print "pt: py scripts init"

# Removes extension
def stripExtension(arg):
	return os.path.splitext(str(arg))[0]

# Lists files matching pattern in path.
def dirlist(*args):
	if len(args) == 2:
		pattern=str(args[0])
		path=str(args[1])
		files=list('') # Seems like a bad way to create a list var

		test=os.path.join(path,pattern)
		entries=glob.glob(test)

		for entry in entries:
			if os.path.isfile(entry):
				files.append(entry)
		return files
	else:
		print "pt: dirlist only accepts two arguments: [pattern] [path]"
		print "args: ",args

# Removed path component to leave only the filename. 
def stripPath(arg):
	return os.path.basename(str(arg))

