if [ "x$#" != "x1" ]; then
	echo 'Usage: mass_replace.sh s/<find>/<replace>/[gi]'
	exit
fi

sources=`find . -iname "*.cc"`
headers=`find . -iname "*.h"`
specials=`find . -iname "*.spec"`
for file in $sources $headers $specials; do 
	echo "Processing: ${file}";
	cp $file $file.old;
    sed $* $file > $file.new; 
    mv $file.new $file; 
done
