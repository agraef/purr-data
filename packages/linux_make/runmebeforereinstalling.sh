#rm -f build/usr/local/lib/pd-l2ork/doc/examples/Gem
#rm -f build/usr/local/lib/pd-l2ork/doc/manuals/Gem
#rm -f build/usr/local/Makefile
#rm -f build/etc/pd-l2ork/default.pdl2ork

mkdir -p build/usr/local/lib/pd-l2ork
ln -s -f pd-l2ork build/usr/local/lib/pd

exit 0

