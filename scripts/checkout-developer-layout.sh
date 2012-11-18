#!/bin/sh

# this script automatically generates a directory with all of the Pd code out
# of CVS in the standard developer's layout.  <hans@at.or.at>

# Usage: 
#	 - with no arguments, it will check out the code using anonymous CVS.
#	 - to check out using your SourceForge ID, add that as the argument

URL="https://pure-data.svn.sourceforge.net/svnroot/pure-data/trunk/"
GEMURL="https://pd-gem.svn.sourceforge.net/svnroot/pd-gem/trunk/"
PDAUTH=""
SVNOPTIONS="--ignore-externals"

print_usage ()
{
    echo " "
    echo "Usage: $0 [sourceforge ID]"
    echo "   if no ID is given, it will check out anonymously"
    echo " "
    exit
}

if [ $# -eq 0 ]; then
    echo "Checking out anonymously. Give your SourceForge ID if you don't want that."
elif [ "$1" == "--help" ]; then
    print_usage
elif [ "$1" == "-h" ]; then
    print_usage
elif [ $# -eq 1 ]; then
    PDAUTH="--username $1"
else
    print_usage
fi

echo "checking out pure-data"
svn checkout $SVNOPTIONS $PDAUTH $URL pure-data

cd pure-data

for section in Gem videoIO; do
         echo "checking out Gem::${section}"
         svn checkout $SVNOPTIONS ${PDAUTH} ${GEMURL}/${section} ${section}
done


# make the symlinks which simulate the files being installed into the packages
cd packages && make devsymlinks
