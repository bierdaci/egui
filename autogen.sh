#! /bin/sh
set -x
aclocal
autoheader
libtoolize -c -f -i
automake --foreign --add-missing --copy
autoconf
