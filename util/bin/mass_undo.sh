for file in `find . -iname *.old`; do 
	original=`echo "$file" | sed 's/\(\)\(.old\)/\1/g'`
	echo "Moving: ${file} to ${original}";
    mv $file $original; 
done
