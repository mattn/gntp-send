#!/bin/sh

libtoolize --copy --force
aclocal
automake --add-missing
autoconf --force
