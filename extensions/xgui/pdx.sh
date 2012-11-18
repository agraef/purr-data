#!/bin/sh
if test "x${PD}" = "x"; then
 PD=../bin/pd
fi
./bin/xgui.sh &
${PD} -open ./main/pdx_connect.pd \
 -path ./main/ \
 -path ./adapters_in \
 -path ./adapters_out \
 -path ./behaviors \
 -path ./filters \
 -path ./objects \
 -path ./utils \
 -path ./physics \
 -path ./bin

 
