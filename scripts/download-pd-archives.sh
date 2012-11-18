#!/bin/sh

# This script downloads mbox archives of the Pd lists and sets them up at
# Apple Mail.app local mailboxes.
# <hans@at.or.at>

# which lists you want to download.  The lists hosted on this server are:
#     gem-dev pd-announce pd-cvs pd-dev pd-list pd-ot pdweb
LISTS="pd-announce pd-dev pd-list pd-ot pdweb"
MAILBOX_ROOT=~/Library/Mail/Mailboxes/Pd

for listname in $LISTS ; do
	 echo " "
	 echo " "
	 echo Downloading $listname:
	 if [ ! -d $MAILBOX_ROOT/$listname.mbox ]; then
		  mkdir $MAILBOX_ROOT/$listname.mbox
	 fi
	 cd $MAILBOX_ROOT/$listname.mbox
	 wget http://lists.puredata.info/pipermail/$listname.mbox/$listname.mbox && \
		  (rm mbox table_of_contents; mv $listname.mbox mbox)
done
