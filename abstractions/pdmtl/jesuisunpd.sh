#!/bin/bash

#should detect if on osx

if `test $# -ne 1`
then
	echo "Usage: $0 path_to_your_pd"
fi

externals=`find $1 -type d -name extra`
#echo $externals
if [ $externals ]; then 
  echo "Found extra folder: $externals"
else
  echo "Could not find extra folder"
  exit
fi

docs=`find $1 -type d -name 5.reference`
#echo $docs
if [ $docs ]; then 
  echo "Found docs folder: $docs"
else
  echo "Could not find docs folder"
  exit
fi


folders="cyclone ggee iemmatrix tof toxy flatspace moocow zexy hcs maxlib iemlib ext13 creb unauthorized pdcontainer oscx pdstring"
echo "Do you want to defolderize the following externals: $folders"
echo -n "YES/NO? "

read answer

if [ "$answer" == 'YES' ]; then 
  for folder in $folders; do
      external=$externals/$folder
      doc=$docs/$folder
      	if [ -a $external  ]; then 
		echo "Moving $external > .."
		mv $external/*.* $externals	
	fi

	if [ -a $doc  ]; then 
		echo "Moving $doc > .."
		mv $doc/*.pd $docs
	fi

  done
fi
