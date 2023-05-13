# This is the script that originally generated the Visual Studio project
# to build for Windows. It is kept for educational and historical purposes,
# or just in case the project needs to be regenerated.
# Note that the vcxproj has been significantly modified by hand after this
# script generated it. This will not on its own create an Xcode project
# that will build Blah Blob!

rm BlahBlob.exe
rm BlahBlob.sln
rm BlahBlob.vcxproj
rm BlahBlob.vcxproj.*

rm -rf .vs
rm setup.sh

./setup_t \
-t wx64 \
-api sd2 \
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

# Gotta get rid of some UTF-8 boms for some reason, otherwise it won't
# execute correctly:
sed -i 's/\xEF\xBB\xBF//g' setup.sh

echo Running setup.sh
chmod a+x ./setup.sh
./setup.sh

echo Done
