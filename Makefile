all:
	+make -C build all
	+make -C build check
clean:
	+make -C build clean
check:
	+make -C build check
tags:
	+make -C build tags