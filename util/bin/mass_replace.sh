sources=`find . -iname "*.cc"`
headers=`find . -iname "*.h"`
specials=`find . -iname "*.spec"`
for file in $sources $headers $specials; do 
	echo "Processing: ${file}";
	cp $file $file.old;
    sed $* $file > $file.new; 
    mv $file.new $file; 
done
