#!/bin/bash

die()
{
    echo "$@" >&2
    exit 1
}

aclocal || die "aclocal failed"
autoreconf --install || die "autoreconf failed"
automake --add-missing --force-missing --copy --foreign || die "automake failed"
