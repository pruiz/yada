#!/bin/sh

autoconf
aclocal
autoheader
libtoolize --copy --force
