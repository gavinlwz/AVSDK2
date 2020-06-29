#!/bin/bash --login
path=`dirname "$0"`
cd "$path"

target="$1"

rm -rf build
#gradle jarRelease
./gradlew build

