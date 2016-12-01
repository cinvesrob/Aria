#!/bin/bash


if test -f /etc/debian_version
then
   isdebian=1
   islinux=1
else
   isdebian=0
   islinux=0
fi

files="*.cpp *.m ../../LICENSE.txt buildinfo.txt"

if test $isdebian -eq 1
then

  echo "You are on Debian Linux"

  arch=`dpkg-architecture -qDEB_HOST_GNU_CPU | sed s/i.86/i386/g`
   
  echo "arch is $arch"
  
  files="$files libAria.so libArNetworking.so README.txt README.rtf README.pdf Makefile"

  if test "$arch" = "i386"
  then
     files="$files *.mexlnx"
  else
     files="$files *.mexlnx64"
  fi

  make dist

else
  echo "You are not on Linux, must be Windows."
  
  if test -z "$PROCESSOR_ARCHITEW6432"
  then
	arch=i386
  else
    arch=$PROCESSOR_ARCHITEW6432
  fi
  
  echo "arch is $arch"
  
  files="$files AriaVC10.dll ArNetworkingVC10.dll README.txt"
  cp README.md README.txt
  
  if test "$arch" = "AMD64"
  then
     files="$files *.mexw64"
  else
     files="$files *.mexw32"
  fi

fi

ariaver=`cat ../../dist/version.num | tr -d '\r\n'`
datestamp=`date +%Y%m%d`
version=$ariaver-$datestamp
dir=ArNetworking-matlab-$version-$arch


rm -rf $dir
mkdir $dir
cp $files $dir/

for f in $dir/*.cpp
do
   echo "/**" > $f.tmp
   cat ../../dist/CopyrightHeader.txt >> $f.tmp
   echo "**/" >> $f.tmp
   mv $f.tmp $f
done

for f in $dir/*.m
do
   cat ../../dist/CopyrightHeader.txt | sed 's/^/% /' > $f.tmp
   cat $f >> $f.tmp
   mv $f.tmp $f
done


if test $islinux -eq 1
then
	tar czf $dir.tar.gz $dir
else
    '/Program Files/7-Zip/7z' u -r $dir.zip $dir
fi	
