#!/bin/bash

rm *.AppImage
rm -rf appdir
mkdir -p appdir/usr/bin

cp ../blah-blob-resources/BlahBlob.dsk appdir/usr/bin/BlahBlob.dsk
cp ../blah-blob-resources/MacII.ROM appdir/usr/bin/MacII.ROM
linuxdeploy --appdir=appdir \
    -e BlahBlob \
    -i ../blah-blob-resources/blahblob.png \
    -d ../blah-blob-resources/blahblob.desktop \
    --output appimage
