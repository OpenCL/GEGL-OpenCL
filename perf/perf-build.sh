#!/bin/sh

MAKE_FLAGS="-j4 -k "
REV=$1
CC="ccache gcc"

rm -rf prefix
mkdir prefixes
ln -s prefixes/$REV prefix

if [ -d prefixes/$REV ]; then
  if [ ! -f prefixes/$REV/built ]; then
    rm -rf prefixes/$REV
  fi
fi


if [ ! -d prefixes/$REV ]; then
  mkdir prefixes/$REV || true
  ( 
    cd checkout;
    if [ ! -f Makefile ]; then
      CC=$CC ./autogen.sh --disable-introspection --prefix=`pwd`/../prefix;
    fi;

    make $MAKE_FLAGS ;
    make -k install
    touch `pwd`/../prefix/built
  ) > prefix/build_log 2>&1
fi;



