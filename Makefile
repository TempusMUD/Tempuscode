all:
	+make -C build all
clean:
	+make -C build clean
check:
	+make -C build check
tags:
	+make -C build tags