#!/bin/bash

cat /proc/1/cgroup # Check if we run in Docker; https://github.com/AppImage/AppImageKit/issues/912

. /opt/qt*/bin/qt*-env.sh || true

########################################################################
# Build Plaform Theme for Gtk+
########################################################################

find /opt/qt*/| grep platform 
#apt-get update
#apt-get -y install libgtk-3-dev libnotify-dev qt5113d
#git clone https://github.com/CrimsonAS/gtkplatform
#cd gtkplatform
#qmake
#make -j$(nproc)
#sudo make install 
#cd -

########################################################################
# https://askubuntu.com/a/910143
# https://askubuntu.com/a/748186
# Deploy with linuxdeployqt using
# -extra-plugins=platformthemes/libqgtk2.so,styles/libqgtk2style.so
# At runtime, export QT_QPA_PLATFORMTHEME=gtk2 (Xfce does this itself)
########################################################################

git clone http://code.qt.io/qt/qtstyleplugins.git
cd qtstyleplugins
qmake
make -j$(nproc)
make install 
cd -

########################################################################
# Build Scribus and install to appdir/
########################################################################

cmake . -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_INSTALL_PREFIX=/usr -DWANT_HUNSPELL=1 -DWITH_PODOFO=1 -DWANT_GRAPHICSMAGICK=1 -DWANT_DEBUG=0 -DWANT_SVNVERSION=0 -DWANT_GUI_LANG=en_US
make -j$(nproc)
make DESTDIR=appdir -j$(nproc) install ; find appdir/
cp AppImage-package/AppRun appdir/ ; chmod +x appdir/AppRun
cp ./appdir/usr/share/icons/hicolor/256x256/apps/scribus.png ./appdir/
sed -i -e 's|^Icon=.*|Icon=scribus|g' ./appdir/usr/share/applications/scribus.desktop # Needed?

########################################################################
# Bundle everyhting
# to allow the AppImage to run on older systems as well
########################################################################

cd appdir/

# Bundle all of glibc; this should eventually be done by linuxdeployqt
apt-get update -q
apt-get download libc6
find *.deb -exec dpkg-deb -x {} . \;
rm *deb

# Make absolutely sure it will not load stuff from /lib or /usr
sed -i -e 's|/usr|/xxx|g' lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
sed -i -e 's|/usr/lib|/ooo/ooo|g' lib/x86_64-linux-gnu/ld-linux-x86-64.so.2

# Bundle fontconfig settings
mkdir -p etc/fonts/
cp /etc/fonts/fonts.conf etc/fonts/

# Bundle Python
apt-get download libpython2.7-stdlib python2.7 python2.7-minimal libpython2.7-minimal
find *.deb -exec dpkg-deb -x {} . \;
rm *deb
cd -

########################################################################
# Patch away absolute paths
# FIXME: It would be nice if they were relative
########################################################################

sed -i -e 's|/usr/share/scribus|././/share/scribus|g' appdir/usr/bin/scribus
sed -i -e 's|/usr/lib/scribus|././/lib/scribus|g' appdir/usr/bin/scribus
sed -i -e 's|/usr/share/doc/scribus/|././/share/doc/scribus/|g' appdir/usr/bin/scribus

########################################################################
# Also bundle Tcl/Tk, Tkinter (for Calendar script)
########################################################################

mkdir -p appdir/usr/lib appdir/usr/share
cp /usr/li*/python2.7/lib-dynload/_tkinter.so appdir/usr/ # It is indeed picked up here because we cd there at runtime
cp -r /usr/lib/tcltk appdir/usr/lib/
cp -r /usr/share/tcltk appdir/usr/share/

########################################################################
# Create extra qt.conf in a strange location; FIXME: why is this needed?
########################################################################

mkdir -p appdir/lib/x86_64-linux-gnu/
cat > appdir/lib/x86_64-linux-gnu/qt.conf <<\EOF
# Why is this needed here? Bug?
[Paths]
Prefix = ../../usr
Plugins = plugins
Imports = qml
Qml2Imports = qml
EOF

########################################################################
# Generate AppImage
########################################################################

# Finalize AppDir but do not turn into AppImage just yet
wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
chmod a+x linuxdeployqt-continuous-x86_64.AppImage
ARCH=x86_64 ./linuxdeployqt-continuous-x86_64.AppImage --appimage-extract-and-run appdir/usr/share/applications/scribus.desktop \
-appimage -unsupported-bundle-everything \
-executable=appdir/usr/bin/python2.7 -executable=appdir/usr/_tkinter.so -extra-plugins=platformthemes/libqgtk2.so,styles/libqgtk2style.so