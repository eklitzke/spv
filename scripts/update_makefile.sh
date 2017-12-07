#!/bin/bash
#
# Update sources in Makefile.am

set -eu

cd ./src
SOURCES=()
for f in $(git ls-files -- '*.cc' '*.h'); do
  SOURCES+=("$f")
done
for proto in $(git ls-files -- '*.proto' | sed 's/\.proto$//'); do
  SOURCES+=("$proto.pb.cc" "$proto.pb.h")
done

IFS=$'\n' SORTED=($(sort <<<"${SOURCES[*]}"))
unset IFS

echo "spv_SOURCES = ${SORTED[*]}"
sed -i "s|^spv_SOURCES = .*|spv_SOURCES = ${SORTED[*]}|g" Makefile.am
