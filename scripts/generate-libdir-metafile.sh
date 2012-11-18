#!/bin/sh

# this script is used to generate a mylibrary/mylibrary.pd meta file.  This
# file is read for relevant meta data when a libdir is opened. (That's the
# plan at least) <hans@at.or.at>

# keeps track of where the last bit of text was printed so that new text is
# not printed on top of existing text
Y=10

# Usage: print_pd_text($to_file, $meta_type, $text_to_print)
print_pd_text () 
{
	 file_name="$1"; shift
	 meta_type="$1"; shift
	 echo "#X text 10 $Y ${meta_type} $@;" >> "$file_name"
	 ((Y=Y+20))
}

if [ $# -lt 2 ]; then
	 echo "Usage: $0 BASE_DIR LIBNAME [ meta types ] "
	 echo " "
	 echo "  meta types: "
	 echo "     --author"
	 echo "     --copyright"
	 echo "     --description"
	 echo "     --keywords"
	 echo "     --license"
	 echo "     --version"
	 echo " "
else

BASE_DIR="$1"; shift
LIBNAME="$1"; shift
libdir_file_name="${BASE_DIR}/${LIBNAME}/${LIBNAME}-meta.pd"
# create pd file
touch "${libdir_file_name}"

# create .pd header with subpatch called "META"
echo "#N canvas 10 10 200 200 10;" >> "${libdir_file_name}"
echo "#N canvas 20 20 420 300 META 0;" >> "${libdir_file_name}"
#N canvas 249 280 600 398 loc&precess 0;

# add required meta fields
print_pd_text "${libdir_file_name}" META "this is a prototype of a libdir meta file"
print_pd_text "${libdir_file_name}" NAME ${LIBNAME} 


# get meta data types:
while [ $# -ge 1 ]; do
    case $1 in
		  --author)
				print_pd_text "${libdir_file_name}" AUTHOR "$2"
				;;
		  --copyright)
				print_pd_text "${libdir_file_name}" COPYRIGHT "$2"
				;;
		  --description)
				print_pd_text "${libdir_file_name}" DESCRIPTION "$2"
				;;
		  --keywords)
				print_pd_text "${libdir_file_name}" KEYWORDS "$2"
				;;
		  --license)
				print_pd_text "${libdir_file_name}" LICENSE "$2"
				;;
		  --version)
				print_pd_text "${libdir_file_name}" VERSION "$2"
				;;
		  *)
				echo "ERROR: unknown flag: $1 with data: $2"
				;;
    esac
	 shift
	 shift
done

echo "#X restore 10 10 pd META;" >> "${libdir_file_name}"


fi
