#!/bin/sh

DATE=`date +%Y%m%d%H%M%S`
VER=`git rev-parse --short HEAD`
VER="$DATE-$VER"
echo "Version: $VER\""
echo "char *srs_version(){return \"$VER\";}" > version.c
# touch mod_srs.c # trigger a rebuild
