#!/bin/sh
set -x
mkdir m4
libtoolize --automake --copy --force
aclocal-1.11 || aclocal
autoconf --force
autoheader --force
gtkdocize --copy
automake-1.11 --add-missing --copy --force-missing --foreign || automake --add-missing --copy --force-missing --foreign
./configure --enable-maintainer-mode --enable-debug --prefix=/usr --enable-gtk-doc
