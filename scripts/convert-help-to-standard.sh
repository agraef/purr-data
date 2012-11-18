#!/bin/sh

# this script renames help patches that don't follow the OBJECTNAME-help.pd
# format that has become the standard.
# 
# for more info on the standard, check this thread:
# http://lists.puredata.info/pipermail/pd-dev/2003-09/001519.html
#
# this script is a hack to take care of this until all of the files get
# renamed by their respective maintainers.

for helpfile in help-*.pd ; do
	 if [ -e $helpfile ]; then
		  newhelpfile=`echo $helpfile | sed 's/^help\-\(.*\)\.pd$/\1-help.pd/'`
		  if [ "$helpfile" != "$newhelpfile" ]; then
				echo "  swap $helpfile" "$newhelpfile"
				mv "$helpfile" "$newhelpfile"
		  else
				echo "SAMEFILE $helpfile"
		  fi
	 fi
done

# this isn't used yet
#NON_STANDARD_HELP_FILES=`ls -1 *.pd | \
#       grep -v '^.*-help.pd$' | \
#       grep -v '^help-.*.pd$' | \
#       grep -v '^externals.pd$' | \
#       grep -v 'test' | \
#       grep -v 'example' | \
#       grep -v 'demo' | \
#       grep -v 'x_all_guis' | \
#       grep -v 'all_about_' | \
#       grep -v 'pddp' | \
#       grep -v 'readme' | \
#       grep -v '^.*-list.pd$' | \
#       grep -v '^.*-joystick.pd$'`

#for helpfile in $NON_STANDARD_HELP_FILES ; do
#	 echo NONE $helpfile
#done

