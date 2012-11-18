#!/bin/sh
/usr/local/bin/xgui &
/usr/local/bin/pd -open /usr/local/lib/pdx/main/pdx_connect.pd \
 -path /usr/local/lib/pdx/main/ \
 -path /usr/local/lib/pdx/patch4pdx/

