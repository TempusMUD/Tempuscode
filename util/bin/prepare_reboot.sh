#!/bin/bash

pushd ~realm/TempusMUD
cd bin
cp circle circle.old
gzip -fv circle.old
cd ../src
cvs tag `date +tempus-%Y-%m-%d-%H`
cvs update -d
make
popd
