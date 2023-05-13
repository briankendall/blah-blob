# This is the script that originally generated the Xcode project to build
# for x86_64 macOS. It is kept for educational and historical purposes, or
# just in case the project needs to be regenerated.
# Note that the Xcode project has been significantly modified by hand
# after this script generated it. This will not on its own create an Xcode
# project that will build Blah Blob!

#!/bin/bash

rm -rf BlahBlob.app
rm -rf BlahBlob.xcodeproj
rm -rf DerivedData

rm setup.sh
rm setup_t

clang ../setup/tool.c -o setup_t

if [ $? -ne 0 ]; then
    echo Failure!
    exit 1
fi

./setup_t \
-t mc64 \
-m II \
-hres 512 -vres 342 \
-depth 0 \
-mem 8M \
-magnify 1 \
-speed a \
-sbx 0 \
-n "Blah Blob" \
-an "BlahBlob" \
> ./setup.sh

chmod a+x ./setup.sh
./setup.sh
