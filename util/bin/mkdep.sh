#!/bin/sh

objlist=""

CXX=g++
INCLUDE="../include"
CCFLAGS="-g -Wall -Wcast-qual -I/usr/include/libxml2 -I/usr/local/pgsql/include -I/usr/include/pgsql"
CXXFLAGS="$CCFLAGS -I$INCLUDE"

for file in $*; do
objname="\$(OBJ)/"`basename $file | sed -e 's/\.cc$/\.o/'`
objlist="$objlist $objname"
printf "\$(OBJ)/";
if $CXX ${CXXFLAGS} -MM $file; then
	printf "\t\$(CXX) \$(CXXFLAGS) -o \$@ -c \$<\n"
fi
done;

echo OBJFILES=$objlist
