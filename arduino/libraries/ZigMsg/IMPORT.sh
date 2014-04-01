#!/bin/sh

#
# This script imports files from uracoli into this library.
# To get uracoli sources:
#	cd /tmp
#	hg clone http://hg.savannah.nongnu.org/hgweb/uracoli/
#

if [ $# != 1 ]
then
    echo "usage: $0 <uracolidir>" >&2
    exit 1
fi

URACOLIDIR=$1
DESTDIR=.

#
# Import method
#

# IMPORT="ln -s"
IMPORT="cp"


#
# List of files
#

STATIC_H='
    radio.h
    board.h
    boards
    transceiver.h
    const.h
'

GEN_H='
    board_cfg.h
    atmega_rfa1.h
'

C='
    radio_rfa.c
    trx_rfa.c
    trx_datarate.c
'

#
# First, build doc (may be useful) and generate appropriate header files
#

(
    cd $URACOLIDIR
    scons -c distclean
    scons doc
    scons zigduino
)

#
# Prepare installation
#

(
    cd $DESTDIR
    rm -f $STATIC_H $GEN_H $C
)

#
# Install files
#

for i in $STATIC_H
do
    $IMPORT $URACOLIDIR/Src/Lib/Inc/$i $DESTDIR
done

for i in $GEN_H
do
    $IMPORT $URACOLIDIR/install/inc/$i $DESTDIR
done

for i in $C
do
    (
	echo "#include <config.h>"
	cat $URACOLIDIR/Src/Lib/Rf230/$i
    ) > $DESTDIR/$i
done

echo "#define zigduino 1" > $DESTDIR/config.h
