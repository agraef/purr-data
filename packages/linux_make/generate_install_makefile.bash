#!/bin/bash

ROOT_DIR=`echo $1 | sed 's|/*$||'`
prefix=$2

if [ $# -ne 1 ]; then
	 echo "Usage: $0 ROOT_DIR"
	 exit;
fi

SED=`echo sed "s|${ROOT_DIR}/||"`

function print_file ()
{
	 local my_file=`echo $1 | sed 's/\\\$/\$\$/g'`
	 echo -e "\tinstall -p '$my_file' '\$(prefix)/$my_file'"
}

function print_dir ()
{
	 echo -e "\tinstall -d -m0755 '\$(prefix)/$1'"
}

function post_install ()
{
	echo -e "\tcp -f /usr/local/lib/pd-l2ork/pd-l2ork.png /usr/share/icons/hicolor/128x128/apps/"
	echo -e "\tcp -f /usr/local/lib/pd-l2ork/pd-l2ork-red.png /usr/share/icons/hicolor/128x128/apps/"
	echo -e "\tcp -f /usr/local/lib/pd-l2ork/pd-l2ork-k12.png /usr/share/icons/hicolor/128x128/apps/"
	echo -e "\tcp -f /usr/local/lib/pd-l2ork/pd-l2ork-k12-red.png /usr/share/icons/hicolor/128x128/apps/"
	echo -e "\tcp -f /usr/local/lib/pd-l2ork/text-x-puredata.png /usr/share/icons/hicolor/128x128/mimetypes"
	echo -e "\tcp -f /usr/local/lib/pd-l2ork/pd-l2ork.desktop /usr/share/applications/"
	echo -e "\tcp -f /usr/local/lib/pd-l2ork/pd-l2ork-k12.desktop /usr/share/applications/"
	echo -e "\tcp -f /usr/local/lib/pd-l2ork/pd-l2ork-debug.desktop /usr/share/applications/"
	echo -e "\tcp -f /usr/local/lib/pd-l2ork/pd-l2ork-k12-debug.desktop /usr/share/applications/"
	echo -e "\tgtk-update-icon-cache /usr/share/icons/hicolor"

}

function post_remove ()
{
	echo -e "\trm -f /usr/share/icons/hicolor/128x128/apps/pd-l2ork.png"
	echo -e "\trm -f /usr/share/icons/hicolor/128x128/apps/pd-l2ork-red.png"
	echo -e "\trm -f /usr/share/icons/hicolor/128x128/apps/pd-l2ork-k12.png"
	echo -e "\trm -f /usr/share/icons/hicolor/128x128/apps/pd-l2ork-k12-red.png"
	echo -e "\trm -f /usr/share/icons/hicolor/128x128/mimetypes/text-x-puredata.png"
	echo -e "\trm -f /usr/share/applications/pd-l2ork.desktop"
	echo -e "\trm -f /usr/share/applications/pd-l2ork-k12.desktop"
	echo -e "\trm -f /usr/share/applications/pd-l2ork-debug.desktop"
	echo -e "\trm -f /usr/share/applications/pd-l2ork-k12-debug.desktop"
	echo -e "\tgtk-update-icon-cache /usr/share/icons/hicolor"

}

function traverse_install_tree ()
{
	 for file in `\ls -1d $1/*`; do
		  local target=`echo $file | $SED`
		  if [ -d "$file" ]; then
				print_dir "$target"
				traverse_install_tree "$file"
		  elif [ -f "$file" ]; then
				print_file "$target"
#		  else
#				echo "MYSTERY FILE: $file"
		  fi
	 done
}

function remove_file () 
{
# arg, $n-help.pd causes lots of problems
#	 local my_file=`echo $1 | sed 's|$|\\$|g'`
	 local my_file=`echo $1 | sed 's/\\\$/\$\$/g'`
	 # local my_file=$1
	 echo -e "\trm -f -- '\$(prefix)/$my_file'"
}

function remove_dir ()
{
	 echo -e "\t-rmdir '\$(prefix)/$1'"
}

function uninstall_tree ()
{
	 for file in `\ls -1d $1/*`; do
		  local target=`echo $file | $SED`
		  if [ -d "$file" ]; then
				uninstall_tree "$file"
				remove_dir "$target"
		  elif [ -f "$file" ]; then
				remove_file "$target"
#		  else
#				echo "MYSTERY FILE: $file"
		  fi
	 done
}


echo ""
echo "prefix = /usr/local"
echo ""
echo "default:"
echo -e "\t@echo 'you have to run \"make install\" to install Pd-l2ork'"
echo ""
echo "install:"
traverse_install_tree $ROOT_DIR
post_install

echo ""
echo ""
echo ""
echo "uninstall:"
uninstall_tree $ROOT_DIR
post_remove
