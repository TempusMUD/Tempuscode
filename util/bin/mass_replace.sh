if [ "x$#" != "x1" ]; then
	echo 'Usage: mass_replace.sh s/<find>/<replace>/[gi]'
	exit
fi

sources=`find . -iname "*.cc"`
headers=`find . -iname "*.h"`
specials=`find . -iname "*.spec"`
for file in $sources $headers $specials; do 
	cp $file $file.old;
    sed "$*" $file > $file.new
	if [[ -s $file.new && ! -s $file ]]; then
		mv $file.new $file; 
	else
		echo SED error while processing $file!  Cancelling!
		rm $file.new
		cp $file.old $file
		exit -1
	fi
done
