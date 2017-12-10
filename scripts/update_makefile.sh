#!/bin/bash
#
# Update sources in Makefile.am

set -eu

cd ./src
SOURCES=()
for f in $(git ls-files -- '*.cc' '*.h'); do
  SOURCES+=("$f")
done

IFS=$'\n' SORTED=($(sort <<<"${SOURCES[*]}"))
unset IFS

echo "spv_SOURCES = ${SORTED[*]}"
sed -i "s|^spv_SOURCES = .*|spv_SOURCES = ${SORTED[*]}|g" Makefile.am
