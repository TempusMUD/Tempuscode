dir=`pwd`
sources=`find $pwd -iname "*.cc"`
headers=`find $pwd -iname "*.h"`
specials=`find $pwd -iname "*.spec"`
files="${sources} ${headers} ${specials}"
for file in $files; do 
	echo "Processing: ${file}";
	cp $file $file.old;
    cat $file | sed $* > $file.new; 
    mv $file.new $file; 
done
