#!/bin/sh


#==============================================================================#
# functions

print_usage() {
	 echo "Usage: "
	 echo "To load a config file:"
	 echo "   $0 load CONFIG_NAME"
	 echo " "
	 echo "To save the current config to file:"
	 echo "   $0 save CONFIG_NAME"
	 echo " "
	 echo "To delete the current config:"
	 echo "   $0 delete CONFIG_NAME"
	 echo " "
	 echo "To list existing configs:"
	 echo "   $0 list"
	 echo " "
	 echo "To print the contents of a config:"
	 echo "   $0 print CONFIG_NAME"
	 echo " "
	 echo "To see the difference between the current config and another:"
	 echo "   $0 diff CONFIG_NAME"
	 echo " "
	 echo "To use the .pdrc instead, add '--pdrc':"
	 echo "   $0 --pdrc load CONFIG_NAME"
	 echo "   $0 --pdrc save CONFIG_NAME"
	 echo "   $0 --pdrc delete CONFIG_NAME"
	 echo "   $0 --pdrc list"
	 echo "   $0 --pdrc print CONFIG_NAME"
	 echo "   $0 --pdrc diff CONFIG_NAME"
	 exit
}

#==============================================================================#
# THE PROGRAM

if [ $# -lt 1 ]; then
	 print_usage
else
	 # get the command line arguments
	 if [ $1 == "--pdrc" ]; then
		  CONFIG_DIR=~
		  CONFIG_FILE=.pdrc
		  COMMAND=$2
		  CONFIG_NAME=$3
	 else
		  COMMAND=$1
		  CONFIG_NAME=$2
    # location of pref file that Pd reads
		  case `uname` in
				Darwin)
					 CONFIG_DIR=~/Library/Preferences
					 CONFIG_FILE=org.puredata.pdextended.plist
					 ;;
				Linux)
					 CONFIG_DIR=~
					 CONFIG_FILE=.pdextended
					 ;;
				*)
					 echo "Not supported on this platform."
					 exit
					 ;;
		  esac
	 fi
	 
    # everything happens in this dir
	 cd $CONFIG_DIR
	 
	 selected_file="$CONFIG_DIR/$CONFIG_FILE-$CONFIG_NAME"
	 case $COMMAND in
		  load)
				if [ -e "$selected_file" ]; then
					 test -e "$CONFIG_FILE" && mv -f "$CONFIG_FILE" /tmp
					 cp "$selected_file" "$CONFIG_FILE" && \
						  echo "Pd config \"$selected_file\" loaded." 
				else
					 echo "\"$selected_file\" doesn't exist.  No action taken."
				fi
				;;
		  save)
				if [ -e "$CONFIG_FILE" ]; then
					 cp -f "$CONFIG_FILE" "$selected_file" && \
						  echo "Pd config \"$CONFIG_NAME\" saved." 
				else
					 echo "\"$CONFIG_FILE\" doesn't exist.  No action taken."
				fi
				;;
		  delete)
				if [ -e "$selected_file" ]; then
					 rm -f "$selected_file" && \
						  echo "Pd config \"$selected_file\" deleted." 
				else
					 echo "\"$selected_file\" doesn't exist.  No action taken."
				fi
				;;
 		  list)
				echo "Available configs:"
				pwd
				ls -1 "${CONFIG_FILE}"*
pwd
				;;
		  print)
				if [ "${CONFIG_NAME}" == "" ]; then
					 cat "${CONFIG_FILE}"
				else
					 cat "$selected_file"
				fi
				;;
		  diff)
				diff -uw "${CONFIG_FILE}" "$selected_file"
				;;
		  *) print_usage ;;
	 esac
fi
