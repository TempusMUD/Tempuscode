dir=`pwd`
files=`find $pwd -iname "*.old"`
#echo "Files: $files"
for file in $files; do 
	original=`echo "$file" | sed "s/\(\)\(.old\)/\1/g"`
	echo "Moving: ${file} to ${original}";
    mv $file $original; 
done
