#!/bin/bash
#
# Update sources in Makefile.am

set -eu

cd ./src
SOURCES=$(git ls-files -- '*.cc' '*.h' | sort | xargs echo)
echo "spv_SOURCES = ${SOURCES}"
sed -i "s|^spv_SOURCES = .*|spv_SOURCES = ${SOURCES}|g" Makefile.am
