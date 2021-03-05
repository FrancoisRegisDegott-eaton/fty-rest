#!/bin/sh
set +e

make

libname="libfty_rest.so.1.0.0"

libname_src="src/.libs/$libname"
libname_bios="/usr/lib/bios/$libname"

if [ ! -f "$libname_src" ];
then
  echo "$libname_src not found"
  exit 1
fi

sudo strip "$libname_src"

if [ ! -f "$libname_bios.original" ];
then
  echo "arch $libname_bios.original"
  sudo mv "$libname_bios" "$libname_bios.original"
fi

echo "cp $libname_bios"
sudo cp -f $libname_src $libname_bios

if [ -d "src/web" ];
then
  #echo "cp xml files in /etc/tntnet/bios.d"
  #ls src/web/*xml
  #sudo cp src/web/*xml /etc/tntnet/bios.d
fi

echo "restart tntnet@bios"
sudo systemctl restart tntnet@bios
systemctl status tntnet@bios

exit 0
