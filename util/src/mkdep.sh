#!/bin/sh

objlist=""

for file in $*
do
    objname="\$(OBJ)/"`basename $file | sed -e 's/\.cc$/\.o/'`
    objlist="$objlist $objname"

    echo "$objname : $file \\"

    found=0

    for incl in `grep "#include" $file | awk -F\" '{print $2}'`
    do
	if [ $found = 0 ]
	then
	    printf "\t"
	    found=1
	fi

	if echo $incl | grep "Specs" > /dev/null 2>&1
	then
	    echo -n "$incl "
	else
	    echo -n "\$(INCLUDE)/$incl "
	fi
    done

    printf "\n"
    printf "\t\$(CXX) -c \$(CXXFLAGS) %-20s -o $objname" $file
    printf "\n"

done

echo OBJFILES=$objlist
exit




