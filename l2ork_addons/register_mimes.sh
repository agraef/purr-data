#!/bin/bash
# super-simplistic installer of PD file associations for Gnome Desktop by Ivica Ico Bukvic

sudo cp pd-l2ork.xml /usr/share/mime/packages/
sudo sh pd-l2ork.postinst configure

echo "done."

exit 0

