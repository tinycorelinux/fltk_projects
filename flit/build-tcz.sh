#!/bin/sh
# Build .tcz extension package for version 2.11 and 3.x of Tinycore
BASEDIR=`pwd`
PROG=flit
PROG_CAPS=Flit
COMMENT='System Tray with Applets'

mkdir /tmp/${PROG}
cd /tmp/${PROG}

mkdir usr

# Executable
mkdir -p usr/local/bin
cp ${BASEDIR}/${PROG} usr/local/bin/

# Icon
sudo mkdir -p usr/local/share/pixmaps
#sudo cp ${BASEDIR}/${PROG}.png usr/local/share/pixmaps

# Help file
mkdir -p usr/local/share/doc/flit
cp ${BASEDIR}/${PROG}_help.htm usr/local/share/doc/flit/

# .desktop file for icons on desctop, menu items
mkdir usr/local/share/applications
echo "[Desktop Entry]" > usr/local/share/applications/${PROG}.desktop
echo "Encoding=UTF-8" >> usr/local/share/applications/${PROG}.desktop
echo "Name=${PROG}" >> usr/local/share/applications/${PROG}.desktop
echo "Comment=${COMMENT}" >> usr/local/share/applications/${PROG}.desktop
echo "GenericName=${PROG_CAPS}" >> usr/local/share/applications/${PROG}.desktop
echo "Exec=${PROG}" >> usr/local/share/applications/${PROG}.desktop
#echo "Icon=${PROG}" >> usr/local/share/applications/${PROG}.desktop
echo "Icon=logo" >> usr/local/share/applications/${PROG}.desktop
echo "Terminal=false" >> usr/local/share/applications/${PROG}.desktop
echo "StartupNotify=true" >> usr/local/share/applications/${PROG}.desktop
echo "Type=Application" >> usr/local/share/applications/${PROG}.desktop
echo "Categories=Utility;" >> usr/local/share/applications/${PROG}.desktop
#sudo echo "X-FullPathIcon=/usr/local/share/pixmaps/${PROG}.png" >> usr/local/share/applications/${PROG}.desktop
echo "X-FullPathIcon=/usr/local/share/pixmaps/logo.png" >> usr/local/share/applications/${PROG}.desktop
echo "******************************"
cat usr/local/share/applications/${PROG}.desktop
echo "******************************"

cd /tmp/${PROG}
echo "Packaging the following files..."
find /tmp/${PROG}
echo "******************************"
mksquashfs . ../${PROG}.tcz -noappend

#cd $TMPDIR
find usr -not -type d > ../${PROG}.tcz.list

# Create md5 file
cd /tmp
md5sum ${PROG}.tcz > ${PROG}.tcz.md5.txt

cp /tmp/${PROG}.tcz* ${BASEDIR}
#rm -rf /tmp/${PROG}
rm -rf /tmp/${PROG}.tcz*

echo "Build of .tcz for TC 2.11+ and 3.x is complete."
