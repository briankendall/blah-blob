# This is the script that originally generated the makeful to build for
# x86_64 linux using xlib. It is kept for educational and historical
# purposes, or just in case the project needs to be regenerated.
# Note that the makefile has been significantly modified by hand after
# this script generated it. This will not on its own create a makefile
# that will build Blah Blob!

rm setup.sh

./setup_t \
-t lx64 \
-m II \
-hres 512 -vres 342 \
-depth 0 \
-mem 8M \
-magnify 1 \
-speed a \
-sbx 0 \
-n "Blah Blob" \
-an "BlahBlob" \
> setup.sh

echo Running setup.sh
chmod a+x ./setup.sh
./setup.sh

echo Done
