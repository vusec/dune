#!/bin/sh

set -o errexit

ROOT=`dirname $0`

OPTS=""

if [ -f /usr/bin/ld.gold ] && [ -f /usr/bin/ld.bfd ]; then
    TMP=`readlink -f $ROOT/.tmp`
    mkdir -p $TMP
    ln -sf /usr/bin/ld.bfd $TMP/ld
    OPTS="PATH=$TMP:$PATH"
fi

sudo $OPTS make $*

