#!/bin/bash

rm -rf Blah\ Blob.app
cp -a ../mac-x86_64/Blah\ Blob.app Blah\ Blob.app
rm -rf Blah\ Blob.app/Contents/_CodeSignature

lipo -create ../mac-x86_64/Blah\ Blob.app/Contents/MacOS/Blah\ Blob ../mac-arm64/Blah\ Blob.app/Contents/MacOS/Blah\ Blob -output Blah\ Blob.app/Contents/MacOS/Blah\ Blob
